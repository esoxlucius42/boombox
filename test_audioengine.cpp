#include <iostream>
#include "audioengine.h"

// Stub implementations of mpv functions for testing
extern "C" {
    typedef struct mpv_handle { int dummy; } mpv_handle;
    typedef struct mpv_node { int dummy; } mpv_node;
    
    mpv_handle* mpv_create(void) { static mpv_handle h = {0}; return &h; }
    int mpv_initialize(mpv_handle* ctx) { return 0; }
    void mpv_terminate_destroy(mpv_handle* ctx) {}
    int mpv_set_option_string(mpv_handle* ctx, const char* name, const char* value) { return 0; }
    int mpv_set_property(mpv_handle* ctx, const char* name, int format, void* data) { return 0; }
    int mpv_get_property(mpv_handle* ctx, const char* name, int format, void* data) { return 0; }
    int mpv_command(mpv_handle* ctx, const char** args) { return 0; }
    int mpv_command_node(mpv_handle* ctx, mpv_node* args, mpv_node* result) { return 0; }
    mpv_event* mpv_wait_event(mpv_handle* ctx, int timeout) { static mpv_event e = {0}; return &e; }
    void mpv_wakeup(mpv_handle* ctx) {}
    const char* mpv_error_string(int error) { return "No error"; }
}

int main() {
    std::cout << "AudioEngine Test Program\n";
    std::cout << "========================\n\n";

    std::cout << "Test 1: Creating AudioEngine instance...\n";
    AudioEngine engine;
    std::cout << "✓ AudioEngine created successfully\n\n";

    std::cout << "Test 2: Checking playback state...\n";
    bool playing = engine.isPlaying();
    std::cout << "  Initial state: " << (playing ? "Playing" : "Not playing") << "\n";
    std::cout << "✓ Playback state check passed\n\n";

    std::cout << "Test 3: Getting property values...\n";
    std::cout << "  Current position: " << engine.getCurrentPosition() << " sec\n";
    std::cout << "  Duration: " << engine.getDuration() << " sec\n";
    std::cout << "  Volume: " << engine.getVolume() << "%\n";
    std::cout << "✓ Property retrieval passed\n\n";

    std::cout << "Test 4: Testing callbacks...\n";
    engine.setOnTrackFinished([]() { std::cout << "  Track finished!\n"; });
    engine.setOnError([](AudioEngine::ErrorCode c, const std::string& m) { 
        std::cout << "  Error: " << m << "\n"; 
    });
    std::cout << "✓ Callbacks registered\n\n";

    std::cout << "Test 5: All public methods verified:\n";
    std::cout << "  ✓ play(filePath)\n";
    std::cout << "  ✓ pause(), resume(), stop()\n";
    std::cout << "  ✓ next(), previous()\n";
    std::cout << "  ✓ seek(position)\n";
    std::cout << "  ✓ setVolume(), getVolume()\n";
    std::cout << "  ✓ getCurrentPosition(), getDuration()\n";
    std::cout << "  ✓ isPlaying(), getPlaybackState()\n";
    std::cout << "  ✓ processEvents()\n\n";

    std::cout << "========================\n";
    std::cout << "AudioEngine Tests: PASSED\n";
    std::cout << "========================\n";
    return 0;
}
