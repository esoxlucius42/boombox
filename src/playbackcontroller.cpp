#include "playbackcontroller.h"
#include "logger.h"
#include <QFileInfo>
#include <chrono>

PlaybackController::PlaybackController(QObject *parent)
    : QObject(parent),
      audioEngine(std::make_unique<AudioEngine>()),
      fileManager(std::make_unique<FileManager>()) {
    
    try {
        // Seed the random generator with current time
        auto seed = std::chrono::system_clock::now().time_since_epoch().count();
        randomGenerator.seed(seed);
        
        // Connect audio engine signals
        audioEngine->setOnTrackFinished([this]() {
            this->onTrackFinished();
        });
        
        audioEngine->setOnError([this](AudioEngine::ErrorCode code, const std::string& msg) {
            this->onPlaybackError(code, msg);
        });
        
        if (!isBackendAvailable()) {
            Logger::warn("PlaybackController", "Playback backend is unavailable; playback will be disabled");
        } else {
            audioEngine->setVolume(FIXED_VOLUME_LEVEL);
        }

        Logger::info("PlaybackController", "PlaybackController initialized");
    } catch (const std::exception& e) {
        Logger::error("PlaybackController", QString("Exception in constructor: %1").arg(e.what()));
    } catch (...) {
        Logger::error("PlaybackController", "Unknown exception in constructor");
    }
}

PlaybackController::~PlaybackController() {
    if (audioEngine) {
        audioEngine->stop();
    }
    Logger::info("PlaybackController", "PlaybackController destroyed");
}

void PlaybackController::loadFolder(const QString& folderPath) {
    try {
        Logger::info("PlaybackController", QString("Loading folder: %1").arg(folderPath));
        
        // Check if folder path is valid
        QFileInfo folderInfo(folderPath);
        if (!folderInfo.exists() || !folderInfo.isDir()) {
            QString errorMsg = QString("Folder does not exist or is not accessible: %1").arg(folderPath);
            Logger::error("PlaybackController", errorMsg);
            emit playbackError(errorMsg);
            return;
        }
        
        // Check permissions
        if (!folderInfo.isReadable()) {
            QString errorMsg = QString("Permission denied: cannot read folder %1").arg(folderPath);
            Logger::error("PlaybackController", errorMsg);
            emit playbackError(errorMsg);
            return;
        }
        
        // Load folder into FileManager
        if (!fileManager->loadFolder(folderPath)) {
            QString errorMsg = QString("Failed to load folder: %1").arg(folderPath);
            Logger::error("PlaybackController", errorMsg);
            emit playbackError(errorMsg);
            return;
        }
        
        int trackCount = fileManager->getTrackCount();
        Logger::info("PlaybackController", QString("Folder loaded with %1 tracks").arg(trackCount));
        
        if (trackCount == 0) {
            QString errorMsg = "No audio files found in folder";
            Logger::error("PlaybackController", errorMsg);
            emit playbackError(errorMsg);
            return;
        }

        if (!isBackendAvailable()) {
            Logger::warn("PlaybackController", "Folder loaded but backend unavailable; skipping autoplay");
            emitBackendUnavailableErrorOnce();
            return;
        }
        
        // Start with a random track (not index 0)
        int randomIndex = pickRandomTrack();
        playTrackAt(randomIndex);
    } catch (const std::exception& e) {
        QString errorMsg = QString("Exception while loading folder: %1").arg(e.what());
        Logger::error("PlaybackController", errorMsg);
        emit playbackError(errorMsg);
    } catch (...) {
        QString errorMsg = "Unknown exception while loading folder";
        Logger::error("PlaybackController", errorMsg);
        emit playbackError(errorMsg);
    }
}

void PlaybackController::playNext() {
    try {
        if (!isBackendAvailable()) {
            emitBackendUnavailableErrorOnce();
            return;
        }

        int trackCount = fileManager->getTrackCount();
        if (trackCount == 0) {
            Logger::warn("PlaybackController", "playNext called but track list is empty");
            QString errorMsg = "No tracks available to play";
            emit playbackError(errorMsg);
            return;
        }
        
        const int nextIndex = pickRandomTrack();
        playTrackAt(nextIndex);
    } catch (const std::exception& e) {
        Logger::error("PlaybackController", QString("Exception in playNext: %1").arg(e.what()));
    } catch (...) {
        Logger::error("PlaybackController", "Unknown exception in playNext");
    }
}

void PlaybackController::seek(int position) {
    if (position < 0) {
        Logger::warn("PlaybackController", QString("Invalid seek position: %1").arg(position));
        return;
    }
    audioEngine->seek(static_cast<double>(position));
}

bool PlaybackController::isPlaying() const {
    return audioEngine->isPlaying();
}

void PlaybackController::play() {
    try {
        if (!isBackendAvailable()) {
            emitBackendUnavailableErrorOnce();
            return;
        }

        QString currentTrack = fileManager->getCurrentTrack();
        if (currentTrack.isEmpty()) {
            Logger::warn("PlaybackController", "No track to play");
            emit playbackError("No track loaded");
            return;
        }

        if (audioEngine->getPlaybackState() == AudioEngine::PlaybackState::Paused) {
            audioEngine->setVolume(FIXED_VOLUME_LEVEL);
            audioEngine->resume();
            Logger::info("PlaybackController", "Playback resumed");
            return;
        }

        audioEngine->setVolume(FIXED_VOLUME_LEVEL);
        audioEngine->play(currentTrack.toStdString());
        Logger::info("PlaybackController", QString("Playing: %1").arg(currentTrack));
    } catch (const std::exception& e) {
        Logger::error("PlaybackController", QString("Exception in play: %1").arg(e.what()));
    } catch (...) {
        Logger::error("PlaybackController", "Unknown exception in play");
    }
}

void PlaybackController::pause() {
    audioEngine->pause();
    Logger::info("PlaybackController", "Playback paused");
}

int PlaybackController::pickRandomTrack() {
    int trackCount = fileManager->getTrackCount();
    if (trackCount == 0) {
        return -1;
    }
    
    if (trackCount == 1) {
        return 0;
    }
    
    // Generate random index from 0 to trackCount-1
    std::uniform_int_distribution<int> distribution(0, trackCount - 1);
    int randomIndex = distribution(randomGenerator);
    
    Logger::debug("PlaybackController", 
        QString("Picked random track: index %1 of %2").arg(randomIndex).arg(trackCount));
    
    return randomIndex;
}

void PlaybackController::playTrackAt(int index) {
    try {
        if (!isBackendAvailable()) {
            emitBackendUnavailableErrorOnce();
            return;
        }

        if (index < 0 || index >= fileManager->getTrackCount()) {
            Logger::error("PlaybackController", QString("Invalid track index: %1").arg(index));
            emit playbackError("Invalid track index");
            return;
        }
        
        QString trackPath = fileManager->getTrackByPosition(index);
        if (trackPath.isEmpty()) {
            Logger::error("PlaybackController", QString("Failed to get track at index %1").arg(index));
            emit playbackError("Failed to retrieve track");
            return;
        }

        if (!fileManager->setCurrentTrackPosition(index)) {
            Logger::error("PlaybackController", QString("Failed to set current track index: %1").arg(index));
            emit playbackError("Failed to update current track index");
            return;
        }
        
        // Start playing the track
        Logger::info("PlaybackController", QString("Playing track: %1").arg(trackPath));
        audioEngine->setVolume(FIXED_VOLUME_LEVEL);
        audioEngine->play(trackPath.toStdString());
        
        // Emit signal for UI update
        emit trackChanged(trackPath);
        
        // Load and emit metadata
        AudioMetadata meta = fileManager->getMetadata(trackPath);
        emit trackMetadataLoaded(meta);
    } catch (const std::exception& e) {
        Logger::error("PlaybackController", QString("Exception in playTrackAt: %1").arg(e.what()));
        emit playbackError("Error playing track");
    } catch (...) {
        Logger::error("PlaybackController", "Unknown exception in playTrackAt");
        emit playbackError("Unknown error playing track");
    }
}

double PlaybackController::getCurrentPosition() const {
    if (!audioEngine) {
        return 0.0;
    }
    return audioEngine->getCurrentPosition();
}

double PlaybackController::getDuration() const {
    if (!audioEngine) {
        return 0.0;
    }
    return audioEngine->getDuration();
}

int PlaybackController::getCurrentTrackPosition() const {
    if (!fileManager) {
        return -1;
    }
    return fileManager->getCurrentTrackPosition();
}

int PlaybackController::getTrackCount() const {
    if (!fileManager) {
        return 0;
    }
    return fileManager->getTrackCount();
}

void PlaybackController::onTrackFinished() {
    Logger::info("PlaybackController", "Track finished");
    // Automatically play next track
    playNext();
}

void PlaybackController::onPlaybackError(AudioEngine::ErrorCode errorCode, const std::string& errorMsg) {
    try {
        const QString rawErrorMsg = QString::fromStdString(errorMsg);
        const bool backendUnavailableError =
            !isBackendAvailable() ||
            errorCode == AudioEngine::ErrorCode::InitializationFailed ||
            (errorCode == AudioEngine::ErrorCode::PlaybackFailed &&
             rawErrorMsg.contains("AudioEngine not initialized", Qt::CaseInsensitive));

        if (backendUnavailableError) {
            Logger::error("PlaybackController", QString("Playback backend unavailable: %1").arg(rawErrorMsg));
            emitBackendUnavailableErrorOnce();
            return;
        }

        QString currentTrack = fileManager->getCurrentTrack();
        QString error = QString("Playback error for '%1' (code %2): %3")
            .arg(currentTrack)
            .arg(static_cast<int>(errorCode))
            .arg(rawErrorMsg);
        
        Logger::error("PlaybackController", error);
        emit playbackError(error);
        
        // Mark the current file as problematic
        if (!currentTrack.isEmpty()) {
            fileManager->markFileAsProblematic(currentTrack);
        }
        
        // Log error details by type
        switch (errorCode) {
            case AudioEngine::ErrorCode::FileNotFound:
                Logger::warn("PlaybackController", QString("File not found or deleted: %1").arg(currentTrack));
                break;
            case AudioEngine::ErrorCode::CorruptedFile:
                Logger::warn("PlaybackController", QString("File appears corrupted: %1").arg(currentTrack));
                break;
            case AudioEngine::ErrorCode::UnsupportedCodec:
                Logger::warn("PlaybackController", QString("Unsupported codec in: %1").arg(currentTrack));
                break;
            case AudioEngine::ErrorCode::DeviceError:
                Logger::error("PlaybackController", "Audio device error - playback device may be disconnected");
                emit playbackError("Audio device error: output device may be disconnected");
                return;  // Don't skip on device error
            default:
                Logger::warn("PlaybackController", QString("Playback failed for: %1").arg(currentTrack));
                break;
        }
        
        // Try to play next track on error (skip current problematic one)
        playNext();
    } catch (const std::exception& e) {
        Logger::error("PlaybackController", QString("Exception in onPlaybackError: %1").arg(e.what()));
    } catch (...) {
        Logger::error("PlaybackController", "Unknown exception in onPlaybackError");
    }
}

bool PlaybackController::isBackendAvailable() const {
    return audioEngine && audioEngine->isInitialized();
}

void PlaybackController::emitBackendUnavailableErrorOnce() {
    if (backendUnavailableErrorShown) {
        return;
    }

    backendUnavailableErrorShown = true;
    Logger::error("PlaybackController", BACKEND_UNAVAILABLE_MESSAGE);
    emit playbackError(BACKEND_UNAVAILABLE_MESSAGE);
}
