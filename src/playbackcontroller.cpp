#include "playbackcontroller.h"

#include "audioengine.h"
#include "gstaudioengine.h"
#include "logger.h"

#include <QFileInfo>
#include <QFile>
#include <QImage>
#include <QMetaObject>
#include <QPainter>
#include <QThread>
#include <QTimer>
#include <QSize>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <random>
#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

struct SpectrumFrameBuffer {
    mutable std::mutex mutex;
    QImage frame;
    QSize renderSize = QSize(320, 88);
    bool dirty = false;
};

namespace {
constexpr const char* kBackendUnavailableMessage =
    "GStreamer playback initialization failed. Check that the required GStreamer plugins are installed.";
constexpr int kFixedVolumeLevel = 100;
constexpr qint64 kRamBufferChunkSize = 1024 * 1024;
constexpr int kPlaybackWorkerPollingIntervalMs = 250;
constexpr int kSpectrumFrameIntervalMs = 25;
constexpr int kSpectrumSegmentCount = 12;
constexpr int kSpectrumOuterMargin = 6;
constexpr int kSpectrumBarGap = 4;
constexpr int kSpectrumBlockGap = 2;
constexpr QSize kSpectrumFrameSize(320, 88);

QColor spectrumSegmentColor(int blockIndex, int totalBlocks)
{
    const QColor lowColor(102, 206, 255);
    const QColor highColor(255, 216, 114);
    const qreal t = totalBlocks <= 1 ? 0.0 : static_cast<qreal>(blockIndex) / static_cast<qreal>(totalBlocks - 1);

    return QColor::fromRgbF(lowColor.redF() + (highColor.redF() - lowColor.redF()) * t,
                            lowColor.greenF() + (highColor.greenF() - lowColor.greenF()) * t,
                            lowColor.blueF() + (highColor.blueF() - lowColor.blueF()) * t,
                            1.0);
}

QImage renderSpectrumFrame(const SpectrumLevels& levels, const QSize& size)
{
    const QSize renderSize = size.isValid() ? size : kSpectrumFrameSize;
    QImage frame(renderSize, QImage::Format_ARGB32_Premultiplied);
    frame.fill(Qt::transparent);

    QPainter painter(&frame);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF outerRect =
        QRectF(frame.rect()).adjusted(kSpectrumOuterMargin, kSpectrumOuterMargin, -kSpectrumOuterMargin, -kSpectrumOuterMargin);
    if (outerRect.isEmpty()) {
        return frame;
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(18, 18, 18, 110));
    painter.drawRoundedRect(outerRect, 8.0, 8.0);

    painter.setPen(QColor(255, 255, 255, 40));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(outerRect, 8.0, 8.0);

    const QRectF contentRect = outerRect.adjusted(8.0, 6.0, -8.0, -6.0);
    const qreal barWidth =
        (contentRect.width() - static_cast<qreal>(SpectrumLevels::kBandCount - 1) * kSpectrumBarGap) /
        static_cast<qreal>(SpectrumLevels::kBandCount);
    const qreal blockHeight =
        (contentRect.height() - static_cast<qreal>(kSpectrumSegmentCount - 1) * kSpectrumBlockGap) /
        static_cast<qreal>(kSpectrumSegmentCount);

    painter.setPen(Qt::NoPen);
    for (int band = 0; band < SpectrumLevels::kBandCount; ++band) {
        const qreal left = contentRect.left() + band * (barWidth + kSpectrumBarGap);
        int activeBlocks = static_cast<int>(std::lround(levels.bands[band] * static_cast<float>(kSpectrumSegmentCount)));
        if (levels.bands[band] > 0.04F && activeBlocks == 0) {
            activeBlocks = 1;
        }
        activeBlocks = std::clamp(activeBlocks, 0, kSpectrumSegmentCount);

        for (int block = 0; block < kSpectrumSegmentCount; ++block) {
            const int reverseIndex = kSpectrumSegmentCount - 1 - block;
            const qreal top = contentRect.top() + reverseIndex * (blockHeight + kSpectrumBlockGap);
            const QRectF blockRect(left, top, std::max<qreal>(2.0, barWidth), std::max<qreal>(2.0, blockHeight));
            const bool isActive = levels.active && block < activeBlocks;
            painter.setBrush(isActive ? spectrumSegmentColor(block, kSpectrumSegmentCount) : QColor(255, 255, 255, 24));
            painter.drawRoundedRect(blockRect, 1.4, 1.4);
        }
    }

    return frame;
}
int createMemfdHandle(const QString& name) {
#ifdef MFD_CLOEXEC
    const QByteArray utf8Name = name.toUtf8();
    const int fd = static_cast<int>(::syscall(SYS_memfd_create, utf8Name.constData(), MFD_CLOEXEC));
    return fd;
#else
    Q_UNUSED(name);
    errno = ENOSYS;
    return -1;
#endif
}
}

class PlaybackWorker : public QObject {
    Q_OBJECT

public:
    explicit PlaybackWorker(std::shared_ptr<SpectrumFrameBuffer> spectrumFrameBuffer, QObject* parent = nullptr)
        : QObject(parent)
        , mSpectrumFrameBuffer(std::move(spectrumFrameBuffer)) {
    }

public slots:
    void onThreadStarted() {
        if (audioEngine || fileManager) {
            return;
        }

        try {
            auto seed = std::chrono::system_clock::now().time_since_epoch().count();
            randomGenerator.seed(seed);
            backendUnavailableErrorShown = false;

            fileManager = std::make_unique<FileManager>();
            audioEngine = createAudioEngine();
            gstAudioEngine = dynamic_cast<GstAudioEngine*>(audioEngine.get());

            audioEngine->setOnTrackFinished([this]() {
                onTrackFinished();
            });

            audioEngine->setOnFileLoaded([this]() {
                onFileLoaded();
            });

            audioEngine->setOnError([this](AudioEngine::ErrorCode code, const std::string& msg) {
                onPlaybackError(code, msg);
            });

            audioEventTimer = new QTimer(this);
            audioEventTimer->setInterval(kPlaybackWorkerPollingIntervalMs);
            connect(audioEventTimer, &QTimer::timeout, this, &PlaybackWorker::onAudioEventTick);
            audioEventTimer->start();

            spectrumFrameTimer = new QTimer(this);
            spectrumFrameTimer->setInterval(kSpectrumFrameIntervalMs);
            connect(spectrumFrameTimer, &QTimer::timeout, this, &PlaybackWorker::onSpectrumFrameTick);
            spectrumFrameTimer->start();

            if (!isBackendAvailable()) {
                Logger::error("PlaybackWorker", kBackendUnavailableMessage);
            } else {
                audioEngine->setVolume(kFixedVolumeLevel);
            }
            Logger::info("PlaybackWorker",
                         QString("Playback worker polling interval set to %1ms for test build")
                             .arg(kPlaybackWorkerPollingIntervalMs));

            emit playbackSnapshotUpdated(audioEngine->isPlaying(),
                                         audioEngine->getCurrentPosition(),
                                         audioEngine->getDuration());
            writeSpectrumFrame(renderSpectrumFrame(SpectrumLevels{}, currentRenderSize()));
            Logger::info("PlaybackWorker", "Playback worker initialized on dedicated thread");
        } catch (const std::exception& e) {
            Logger::error("PlaybackWorker", QString("Exception in onThreadStarted: %1").arg(e.what()));
            emit playbackError(QString("Failed to initialize playback worker: %1").arg(e.what()));
        } catch (...) {
            Logger::error("PlaybackWorker", "Unknown exception in onThreadStarted");
            emit playbackError("Failed to initialize playback worker");
        }
    }

    void onShutdown() {
        if (audioEventTimer) {
            audioEventTimer->stop();
            audioEventTimer->deleteLater();
            audioEventTimer = nullptr;
        }
        if (spectrumFrameTimer) {
            spectrumFrameTimer->stop();
            spectrumFrameTimer->deleteLater();
            spectrumFrameTimer = nullptr;
        }

        if (audioEngine) {
            audioEngine->stop();
            audioEngine.reset();
        }
        gstAudioEngine = nullptr;

        releaseStagedTrack();
        fileManager.reset();
        emit playbackSnapshotUpdated(false, 0.0, 0.0);
        writeSpectrumFrame(renderSpectrumFrame(SpectrumLevels{}, currentRenderSize()));

        if (QThread* thread = QThread::currentThread()) {
            thread->quit();
        }
    }

    void loadFolder(const QString& folderPath) {
        try {
            Logger::info("PlaybackWorker", QString("Loading folder: %1").arg(folderPath));

            QFileInfo folderInfo(folderPath);
            if (!folderInfo.exists() || !folderInfo.isDir()) {
                emit playbackError(QString("Folder does not exist or is not accessible: %1").arg(folderPath));
                return;
            }

            if (!folderInfo.isReadable()) {
                emit playbackError(QString("Permission denied: cannot read folder %1").arg(folderPath));
                return;
            }

            if (!fileManager || !fileManager->loadFolder(folderPath)) {
                emit playbackError(QString("Failed to load folder: %1").arg(folderPath));
                return;
            }

            const int loadedTrackCount = fileManager->getTrackCount();
            Logger::info("PlaybackWorker", QString("Folder loaded with %1 tracks").arg(loadedTrackCount));

            if (loadedTrackCount == 0) {
                emit playbackError("No audio files found in folder");
                return;
            }

            if (!isBackendAvailable()) {
                Logger::warn("PlaybackWorker", "Folder loaded but GStreamer playback is unavailable; skipping autoplay");
                emitBackendUnavailableErrorOnce();
                emit trackChangedWithContext(fileManager->getCurrentTrack(),
                                             fileManager->getCurrentTrackPosition(),
                                             fileManager->getTrackCount());
                return;
            }

            playTrackAt(pickRandomTrack());
        } catch (const std::exception& e) {
            emit playbackError(QString("Exception while loading folder: %1").arg(e.what()));
        } catch (...) {
            emit playbackError("Unknown exception while loading folder");
        }
    }

    void playNext() {
        try {
            if (!isBackendAvailable()) {
                emitBackendUnavailableErrorOnce();
                return;
            }

            if (!fileManager || fileManager->getTrackCount() == 0) {
                Logger::warn("PlaybackWorker", "playNext called but track list is empty");
                emit playbackError("No tracks available to play");
                return;
            }

            playTrackAt(pickRandomTrack());
        } catch (const std::exception& e) {
            Logger::error("PlaybackWorker", QString("Exception in playNext: %1").arg(e.what()));
        } catch (...) {
            Logger::error("PlaybackWorker", "Unknown exception in playNext");
        }
    }

    void seek(int position) {
        if (position < 0) {
            Logger::warn("PlaybackWorker", QString("Invalid seek position: %1").arg(position));
            return;
        }
        if (audioEngine) {
            audioEngine->seek(static_cast<double>(position));
            emit playbackSnapshotUpdated(audioEngine->isPlaying(),
                                         audioEngine->getCurrentPosition(),
                                         audioEngine->getDuration());
            if (gstAudioEngine) {
                gstAudioEngine->clearSpectrumLevels();
            }
        }
    }

    void play() {
        try {
            if (!isBackendAvailable()) {
                emitBackendUnavailableErrorOnce();
                return;
            }

            const QString currentTrack = fileManager ? fileManager->getCurrentTrack() : QString();
            if (currentTrack.isEmpty()) {
                Logger::warn("PlaybackWorker", "No track to play");
                emit playbackError("No track loaded");
                return;
            }

            if (audioEngine->getPlaybackState() == AudioEngine::PlaybackState::Paused) {
                audioEngine->setVolume(kFixedVolumeLevel);
                audioEngine->resume();
                Logger::info("PlaybackWorker", "Playback resumed");
            } else {
                if (gstAudioEngine) {
                    gstAudioEngine->clearSpectrumLevels();
                }
                audioEngine->setVolume(kFixedVolumeLevel);
                audioEngine->play(currentTrack.toStdString());
                Logger::info("PlaybackWorker", QString("Playing: %1").arg(currentTrack));
            }

            emit playbackSnapshotUpdated(audioEngine->isPlaying(),
                                         audioEngine->getCurrentPosition(),
                                         audioEngine->getDuration());
        } catch (const std::exception& e) {
            Logger::error("PlaybackWorker", QString("Exception in play: %1").arg(e.what()));
        } catch (...) {
            Logger::error("PlaybackWorker", "Unknown exception in play");
        }
    }

    void pause() {
        if (!audioEngine) {
            return;
        }

        audioEngine->pause();
        if (gstAudioEngine) {
            gstAudioEngine->clearSpectrumLevels();
        }
        Logger::info("PlaybackWorker", "Playback paused");
        emit playbackSnapshotUpdated(audioEngine->isPlaying(),
                                     audioEngine->getCurrentPosition(),
                                     audioEngine->getDuration());
    }

private slots:
    void onAudioEventTick() {
        if (!audioEngine) {
            return;
        }

        audioEngine->processEvents();
        const bool nowPlaying = audioEngine->isPlaying();
        emit playbackSnapshotUpdated(nowPlaying,
                                     audioEngine->getCurrentPosition(),
                                     audioEngine->getDuration());
    }

    void onSpectrumFrameTick() {
        if (!gstAudioEngine) {
            return;
        }

        const auto levels = gstAudioEngine->takeLatestSpectrumLevels();
        if (!levels) {
            return;
        }

        writeSpectrumFrame(renderSpectrumFrame(*levels, currentRenderSize()));
    }

private:
    void onTrackFinished() {
        releaseStagedTrack();
        Logger::info("PlaybackWorker", "Track finished");
        playNext();
    }

    void onFileLoaded() {
        if (stagedTrackFd >= 0) {
            Logger::info("PlaybackWorker",
                         QString("Releasing worker RAM-buffer handle after GStreamer loaded track: %1")
                             .arg(stagedTrackSourcePath));
        }
        releaseStagedTrack();
    }

    void onPlaybackError(AudioEngine::ErrorCode errorCode, const std::string& errorMsg) {
        try {
            const QString rawErrorMsg = QString::fromStdString(errorMsg);
            const bool backendUnavailableError =
                !isBackendAvailable() ||
                errorCode == AudioEngine::ErrorCode::InitializationFailed ||
                (errorCode == AudioEngine::ErrorCode::PlaybackFailed &&
                 rawErrorMsg.contains("AudioEngine not initialized", Qt::CaseInsensitive));

            if (backendUnavailableError) {
                releaseStagedTrack();
                Logger::error("PlaybackWorker", QString("GStreamer playback initialization failed: %1").arg(rawErrorMsg));
                emitBackendUnavailableErrorOnce();
                return;
            }

            const QString currentTrack = fileManager ? fileManager->getCurrentTrack() : QString();
            const QString error = QString("Playback error for '%1' (code %2): %3")
                                      .arg(currentTrack)
                                      .arg(static_cast<int>(errorCode))
                                      .arg(rawErrorMsg);

            Logger::error("PlaybackWorker", error);
            emit playbackError(error);
            releaseStagedTrack();

            if (fileManager && !currentTrack.isEmpty()) {
                fileManager->markFileAsProblematic(currentTrack);
            }

            switch (errorCode) {
                case AudioEngine::ErrorCode::FileNotFound:
                    Logger::warn("PlaybackWorker", QString("File not found or deleted: %1").arg(currentTrack));
                    break;
                case AudioEngine::ErrorCode::CorruptedFile:
                    Logger::warn("PlaybackWorker", QString("File appears corrupted: %1").arg(currentTrack));
                    break;
                case AudioEngine::ErrorCode::UnsupportedCodec:
                    Logger::warn("PlaybackWorker", QString("Unsupported codec in: %1").arg(currentTrack));
                    break;
                case AudioEngine::ErrorCode::DeviceError:
                    Logger::error("PlaybackWorker", "Audio device error - playback device may be disconnected");
                    emit playbackError("Audio device error: output device may be disconnected");
                    return;
                default:
                    Logger::warn("PlaybackWorker", QString("Playback failed for: %1").arg(currentTrack));
                    break;
            }

            playNext();
        } catch (const std::exception& e) {
            Logger::error("PlaybackWorker", QString("Exception in onPlaybackError: %1").arg(e.what()));
        } catch (...) {
            Logger::error("PlaybackWorker", "Unknown exception in onPlaybackError");
        }
    }

    bool isBackendAvailable() const {
        return audioEngine && audioEngine->isInitialized();
    }

    void releaseStagedTrack() {
        if (stagedTrackFd >= 0) {
            ::close(stagedTrackFd);
            stagedTrackFd = -1;
        }
        stagedTrackSourcePath.clear();
        stagedTrackPlaybackPath.clear();
    }

    QString stageTrackInRam(const QString& sourcePath) {
        QFile sourceFile(sourcePath);
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            emit playbackError(QString("Failed to open track for RAM buffering: %1").arg(sourcePath));
            return QString();
        }

        Logger::info("PlaybackWorker",
                     QString("Buffering full track into RAM before playback: %1 (%2 bytes)")
                         .arg(sourcePath)
                         .arg(sourceFile.size()));

        const QString memfdName = QFileInfo(sourcePath).fileName().isEmpty()
            ? QStringLiteral("boombox-track")
            : QFileInfo(sourcePath).fileName();
        const int fd = createMemfdHandle(memfdName);
        if (fd < 0) {
            emit playbackError(QString("Failed to create RAM buffer for playback: %1")
                                   .arg(QString::fromLocal8Bit(std::strerror(errno))));
            return QString();
        }

        while (!sourceFile.atEnd()) {
            const QByteArray chunk = sourceFile.read(kRamBufferChunkSize);
            if (chunk.isEmpty() && sourceFile.error() != QFile::NoError) {
                const QString errorText = sourceFile.errorString();
                ::close(fd);
                emit playbackError(QString("Failed while reading track into RAM: %1").arg(errorText));
                return QString();
            }

            const char* data = chunk.constData();
            qint64 remaining = chunk.size();
            while (remaining > 0) {
                const ssize_t written = ::write(fd, data, static_cast<size_t>(remaining));
                if (written < 0) {
                    const QString errorText = QString::fromLocal8Bit(std::strerror(errno));
                    ::close(fd);
                    emit playbackError(QString("Failed while writing RAM buffer: %1").arg(errorText));
                    return QString();
                }
                remaining -= written;
                data += written;
            }
        }

        if (::lseek(fd, 0, SEEK_SET) < 0) {
            const QString errorText = QString::fromLocal8Bit(std::strerror(errno));
            ::close(fd);
            emit playbackError(QString("Failed to rewind RAM buffer: %1").arg(errorText));
            return QString();
        }

        const QString playbackPath = QString("/proc/self/fd/%1").arg(fd);
        Logger::info("PlaybackWorker",
                     QString("Track fully buffered in RAM: %1 -> %2")
                         .arg(sourcePath)
                         .arg(playbackPath));

        releaseStagedTrack();
        stagedTrackFd = fd;
        stagedTrackSourcePath = sourcePath;
        stagedTrackPlaybackPath = playbackPath;
        return stagedTrackPlaybackPath;
    }

    void emitBackendUnavailableErrorOnce() {
        if (backendUnavailableErrorShown) {
            return;
        }

        backendUnavailableErrorShown = true;
        Logger::error("PlaybackWorker", kBackendUnavailableMessage);
        emit playbackError(kBackendUnavailableMessage);
    }

    QSize currentRenderSize() const
    {
        if (!mSpectrumFrameBuffer) {
            return kSpectrumFrameSize;
        }

        std::lock_guard<std::mutex> lock(mSpectrumFrameBuffer->mutex);
        return mSpectrumFrameBuffer->renderSize;
    }

    void writeSpectrumFrame(QImage frame)
    {
        if (!mSpectrumFrameBuffer) {
            return;
        }

        std::lock_guard<std::mutex> lock(mSpectrumFrameBuffer->mutex);
        mSpectrumFrameBuffer->frame = std::move(frame);
        mSpectrumFrameBuffer->dirty = true;
    }

    int pickRandomTrack() {
        const int totalTracks = fileManager ? fileManager->getTrackCount() : 0;
        if (totalTracks <= 0) {
            return -1;
        }

        if (totalTracks == 1) {
            return 0;
        }

        std::uniform_int_distribution<int> distribution(0, totalTracks - 1);
        const int randomIndex = distribution(randomGenerator);
        Logger::debug("PlaybackWorker",
                      QString("Picked random track: index %1 of %2").arg(randomIndex).arg(totalTracks));
        return randomIndex;
    }

    void playTrackAt(int index) {
        try {
            if (!isBackendAvailable()) {
                emitBackendUnavailableErrorOnce();
                return;
            }

            if (!fileManager || index < 0 || index >= fileManager->getTrackCount()) {
                emit playbackError("Invalid track index");
                return;
            }

            const QString trackPath = fileManager->getTrackByPosition(index);
            if (trackPath.isEmpty()) {
                emit playbackError("Failed to retrieve track");
                return;
            }

            if (!fileManager->setCurrentTrackPosition(index)) {
                emit playbackError("Failed to update current track index");
                return;
            }

            Logger::info("PlaybackWorker", QString("Playing track: %1").arg(trackPath));
            const QString playbackPath = stageTrackInRam(trackPath);
            if (playbackPath.isEmpty()) {
                return;
            }

            if (gstAudioEngine) {
                gstAudioEngine->clearSpectrumLevels();
            }
            audioEngine->setVolume(kFixedVolumeLevel);
            audioEngine->play(playbackPath.toStdString());

            emit trackChangedWithContext(trackPath, index, fileManager->getTrackCount());
            emit playbackSnapshotUpdated(audioEngine->isPlaying(),
                                         audioEngine->getCurrentPosition(),
                                         audioEngine->getDuration());

            QTimer::singleShot(0, this, [this, trackPath, index]() {
                if (!fileManager) {
                    return;
                }

                if (fileManager->getCurrentTrackPosition() != index ||
                    fileManager->getTrackByPosition(index) != trackPath) {
                    return;
                }

                const AudioMetadata meta = fileManager->getMetadata(trackPath);
                emit trackMetadataLoaded(meta);
            });
        } catch (const std::exception& e) {
            Logger::error("PlaybackWorker", QString("Exception in playTrackAt: %1").arg(e.what()));
            emit playbackError("Error playing track");
        } catch (...) {
            Logger::error("PlaybackWorker", "Unknown exception in playTrackAt");
            emit playbackError("Unknown error playing track");
        }
    }

signals:
    void trackChangedWithContext(const QString& filePath, int position, int trackCount);
    void trackMetadataLoaded(const AudioMetadata& meta);
    void playbackError(const QString& error);
    void playbackSnapshotUpdated(bool playing, double position, double duration);

private:
    std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<FileManager> fileManager;
    std::mt19937 randomGenerator;
    bool backendUnavailableErrorShown = false;
    QTimer* audioEventTimer = nullptr;
    QTimer* spectrumFrameTimer = nullptr;
    GstAudioEngine* gstAudioEngine = nullptr;
    int stagedTrackFd = -1;
    QString stagedTrackSourcePath;
    QString stagedTrackPlaybackPath;
    std::shared_ptr<SpectrumFrameBuffer> mSpectrumFrameBuffer;
};

PlaybackController::PlaybackController(QObject *parent)
    : QObject(parent) {
    qRegisterMetaType<AudioMetadata>("AudioMetadata");

    spectrumFrameBuffer = std::make_shared<SpectrumFrameBuffer>();
    workerThread = new QThread(this);
    worker = new PlaybackWorker(spectrumFrameBuffer);
    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::started, worker, &PlaybackWorker::onThreadStarted);
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, this, &PlaybackController::onWorkerThreadFinished);

    connect(this, &PlaybackController::requestLoadFolder,
            worker, &PlaybackWorker::loadFolder, Qt::QueuedConnection);
    connect(this, &PlaybackController::requestPlayNext,
            worker, &PlaybackWorker::playNext, Qt::QueuedConnection);
    connect(this, &PlaybackController::requestSeek,
            worker, &PlaybackWorker::seek, Qt::QueuedConnection);
    connect(this, &PlaybackController::requestPlay,
            worker, &PlaybackWorker::play, Qt::QueuedConnection);
    connect(this, &PlaybackController::requestPause,
            worker, &PlaybackWorker::pause, Qt::QueuedConnection);
    connect(this, &PlaybackController::requestShutdown,
            worker, &PlaybackWorker::onShutdown, Qt::QueuedConnection);

    connect(worker, &PlaybackWorker::trackChangedWithContext,
            this, &PlaybackController::onWorkerTrackChanged, Qt::QueuedConnection);
    connect(worker, &PlaybackWorker::trackMetadataLoaded,
            this, &PlaybackController::onWorkerTrackMetadataLoaded, Qt::QueuedConnection);
    connect(worker, &PlaybackWorker::playbackError,
            this, &PlaybackController::onWorkerPlaybackError, Qt::QueuedConnection);
    connect(worker, &PlaybackWorker::playbackSnapshotUpdated,
            this, &PlaybackController::onWorkerPlaybackSnapshot, Qt::QueuedConnection);

    workerThread->start();
    Logger::info("PlaybackController", "PlaybackController initialized with dedicated playback thread");
}

PlaybackController::~PlaybackController() {
    shutdown();
    Logger::info("PlaybackController", "PlaybackController destroyed");
}

void PlaybackController::loadFolder(const QString& folderPath) {
    emit requestLoadFolder(folderPath);
}

void PlaybackController::playNext() {
    emit requestPlayNext();
}

void PlaybackController::seek(int position) {
    if (position < 0) {
        Logger::warn("PlaybackController", QString("Invalid seek position: %1").arg(position));
        return;
    }

    emit requestSeek(position);
}

bool PlaybackController::isPlaying() const {
    return playing;
}

void PlaybackController::play() {
    emit requestPlay();
}

void PlaybackController::pause() {
    emit requestPause();
}

double PlaybackController::getCurrentPosition() const {
    return currentPosition;
}

double PlaybackController::getDuration() const {
    return duration;
}

int PlaybackController::getCurrentTrackPosition() const {
    return currentTrackPosition;
}

int PlaybackController::getTrackCount() const {
    return trackCount;
}

QImage PlaybackController::takeLatestSpectrumFrame() const
{
    if (!spectrumFrameBuffer) {
        return {};
    }

    std::lock_guard<std::mutex> lock(spectrumFrameBuffer->mutex);
    if (!spectrumFrameBuffer->dirty) {
        return {};
    }

    spectrumFrameBuffer->dirty = false;
    return spectrumFrameBuffer->frame;
}

void PlaybackController::shutdown() {
    if (shutdownRequested) {
        return;
    }

    shutdownRequested = true;

    if (!workerThread) {
        return;
    }

    if (workerThread->isRunning()) {
        emit requestShutdown();
        if (!workerThread->wait(5000)) {
            Logger::warn("PlaybackController", "Playback worker thread did not stop within timeout");
            workerThread->quit();
            workerThread->wait(5000);
        }
    }
}

void PlaybackController::onWorkerTrackChanged(const QString& filePath, int position, int totalTracks) {
    currentTrackPosition = position;
    trackCount = totalTracks;
    emit trackChanged(filePath);
}

void PlaybackController::onWorkerTrackMetadataLoaded(const AudioMetadata& meta) {
    emit trackMetadataLoaded(meta);
}

void PlaybackController::onWorkerPlaybackError(const QString& error) {
    emit playbackError(error);
}

void PlaybackController::onWorkerPlaybackSnapshot(bool nowPlaying, double position, double trackDuration) {
    playing = nowPlaying;
    currentPosition = position;
    duration = trackDuration;
}

void PlaybackController::onWorkerThreadFinished() {
    worker = nullptr;
}

#include "playbackcontroller.moc"
