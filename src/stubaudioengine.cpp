#include "stubaudioengine.h"

void StubAudioEngine::play(const std::string& /* filePath */) {
}

void StubAudioEngine::pause() {
}

void StubAudioEngine::resume() {
}

void StubAudioEngine::stop() {
    mState = PlaybackState::Stopped;
}

void StubAudioEngine::next() {
    if (mOnTrackFinished) {
        mOnTrackFinished();
    }
}

void StubAudioEngine::previous() {
}

void StubAudioEngine::seek(double /* positionSeconds */) {
}

void StubAudioEngine::setVolume(int /* level */) {
}

int StubAudioEngine::getVolume() const {
    return 0;
}

double StubAudioEngine::getCurrentPosition() const {
    return 0.0;
}

double StubAudioEngine::getDuration() const {
    return 0.0;
}

bool StubAudioEngine::isPlaying() const {
    return false;
}

bool StubAudioEngine::isInitialized() const {
    return false;
}

AudioEngine::PlaybackState StubAudioEngine::getPlaybackState() const {
    return mState;
}

void StubAudioEngine::setOnTrackFinished(TrackFinishedCallback callback) {
    mOnTrackFinished = std::move(callback);
}

void StubAudioEngine::setOnFileLoaded(FileLoadedCallback callback) {
    mOnFileLoaded = std::move(callback);
}

void StubAudioEngine::setOnError(ErrorCallback callback) {
    mOnError = std::move(callback);
}

void StubAudioEngine::processEvents() {
}
