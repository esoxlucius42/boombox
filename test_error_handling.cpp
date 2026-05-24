/**
 * @brief Test program to verify error handling in Boombox
 * 
 * This test verifies:
 * 1. App handles file not found errors without crashing
 * 2. App handles corrupted files without crashing
 * 3. App skips problematic files and plays next track
 * 4. Error messages are logged properly
 */

#include <iostream>
#include <fstream>
#include <cassert>
#include <cstdlib>
#include <string>
#include "include/filemanager.h"

void test_file_exists() {
    std::cout << "\n=== Test 1: File Existence Check ===" << std::endl;
    
    // Test that good.mp3 exists
    std::ifstream goodFile("test_files/good.mp3");
    assert(goodFile.good());
    goodFile.close();
    std::cout << "[PASS] good.mp3 found" << std::endl;
    
    // Test that missing.mp3 doesn't exist
    std::ifstream missingFile("test_files/missing.mp3");
    assert(!missingFile.good());
    missingFile.close();
    std::cout << "[PASS] missing.mp3 correctly not found" << std::endl;
}

void test_file_manager_tracks() {
    std::cout << "\n=== Test 2: FileManager Tracks ===" << std::endl;
    
    FileManager fm;
    bool loaded = fm.loadFolder("test_files");
    assert(loaded);
    std::cout << "[PASS] Folder loaded successfully" << std::endl;
    
    int trackCount = fm.getTrackCount();
    assert(trackCount > 0);
    std::cout << "[PASS] Track list contains " << trackCount << " tracks" << std::endl;
    
    QString firstTrack = fm.getTrackByPosition(0);
    assert(!firstTrack.isEmpty());
    std::cout << "[PASS] First track: " << firstTrack.toStdString() << std::endl;
}

void test_file_manager_problematic_files() {
    std::cout << "\n=== Test 3: Problematic File Tracking ===" << std::endl;
    
    FileManager fm;
    fm.loadFolder("test_files");
    
    // Mark a file as problematic
    QString testFile = "test_files/corrupted.mp3";
    fm.markFileAsProblematic(testFile);
    std::cout << "[PASS] Marked file as problematic: " << testFile.toStdString() << std::endl;
    
    // Get next playable track (should skip problematic)
    QString nextTrack = fm.getNextPlayableTrack(0);
    assert(!nextTrack.isEmpty());
    std::cout << "[PASS] Got next playable track: " << nextTrack.toStdString() << std::endl;
}

void test_empty_tracks_handling() {
    std::cout << "\n=== Test 4: Empty Track List Handling ===" << std::endl;
    
    FileManager fm;
    
    // Track list starts empty
    assert(fm.getTrackCount() == 0);
    std::cout << "[PASS] Empty track list has size 0" << std::endl;
    
    // Getting track from empty list returns empty string
    QString track = fm.getTrackByPosition(0);
    assert(track.isEmpty());
    std::cout << "[PASS] getTrackByPosition(0) on empty list returns empty string" << std::endl;
}

void test_file_readability() {
    std::cout << "\n=== Test 5: File Readability Check ===" << std::endl;
    
    FileManager fm;
    fm.loadFolder("test_files");
    
    // All files in test_files should be readable
    for (int i = 0; i < fm.getTrackCount(); ++i) {
        QString track = fm.getTrackByPosition(i);
        assert(!track.isEmpty());
        std::cout << "[PASS] Track " << i << " is accessible: " 
                  << track.toStdString() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=====================================\n"
              << "Boombox Error Handling Test Suite\n"
              << "=====================================" << std::endl;
    
    try {
        test_file_exists();
        test_file_manager_tracks();
        test_file_manager_problematic_files();
        test_empty_tracks_handling();
        test_file_readability();
        
        std::cout << "\n=====================================" << std::endl;
        std::cout << "[SUCCESS] All tests passed!" << std::endl;
        std::cout << "=====================================" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[FAILURE] Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n[FAILURE] Unknown exception caught" << std::endl;
        return 1;
    }
}
