# FileManager Implementation Summary

## Overview
Implemented a lazy-loading file discovery and queue management system for Boombox audio player.

## Architecture

### Core Classes
- **FileManager**: Main class managing file discovery, queue building, and metadata loading
- **AudioMetadata**: Data structure holding track metadata (title, artist, album, duration)

### Key Design Decisions

1. **Lazy Metadata Loading**: 
   - Metadata is only loaded when `getMetadata()` is called for a specific file
   - Results are cached in `metadataCache` to avoid re-reading
   - This prevents the player from stalling while loading thousands of files

2. **Recursive Folder Scanning**:
   - Uses Qt's `QDir` for cross-platform file operations
   - Recursively discovers all audio files in selected folder tree
   - Sorts queue alphabetically for consistent ordering

3. **Format Support**:
   - MP3, FLAC, WAV, OGG, M4A, AAC, WMA, APE
   - Format detection uses file extensions

## Public API

```cpp
bool loadFolder(const QString& folderPath)
    - Scan folder recursively, build queue, return success

int getQueueSize() const
    - Number of files in queue

QString getCurrentTrack() const
    - Get full path of current track

int getCurrentTrackIndex() const
    - Get position in queue (0-based)

QString getNextTrack() const
    - Peek at next track without advancing

void advanceQueue()
    - Move to next track (wraps to beginning at end)

void regressQueue()
    - Move to previous track (wraps to end at beginning)

QString getTrackAt(int index) const
    - Get specific track by index

AudioMetadata getMetadata(const QString& filePath)
    - Load/retrieve metadata for file (with caching)
```

## Implementation Details

### File Scanning
- `loadFolder()`: Entry point, clears queue, initiates recursive scan
- `loadFolderRecursive()`: Recursively processes subdirectories
- `isSupportedAudioFormat()`: Checks file extension against supported list
- Queue stored as `QStringList`, automatically sorted alphabetically

### Metadata Loading
- `getMetadata()`: Checks cache first, then extracts if needed
- `extractMetadata()`: Attempts to read ID3 tags from MP3 files
- Gracefully handles missing metadata with sensible defaults

### Queue Navigation
- Current index tracked by `currentIndex` member variable
- Advance/regress wrap around at queue boundaries
- Invalid indices return empty strings

## Testing

Created comprehensive test that verifies:
- ✓ Folder loading with 3 audio files
- ✓ Correct queue size
- ✓ Current track retrieval
- ✓ Queue advancement
- ✓ Queue regression
- ✓ Metadata extraction
- ✓ Metadata caching

All tests pass successfully.

## Files Modified
- `include/filemanager.h` - Class declaration
- `src/filemanager.cpp` - Implementation
- `CMakeLists.txt` - Added source file to build
- `src/main.cpp` - Added test code

## Status
Implementation complete and fully functional. Lazy-loading architecture prevents performance issues with large music libraries.
