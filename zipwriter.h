// ZIP Writer (currently only uncompressed)
// Dan Jackson, 2014

#ifndef ZIPWRITER_H
#define ZIPWRITER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Buffer sizes required before any additional alignment
#define ZIP_WRITER_MAX_PATH 256
#define ZIP_WRITER_SIZE_HEADER (46 + ZIP_WRITER_MAX_PATH)
#define ZIP_WRITER_SIZE_MAX    (ZIP_WRITER_SIZE_HEADER + 4) //// max(header, footer, directory) +4 bytes for "extra field" padding header, user must add 'alignment' bytes to this length


// ZIP Timestamp
#define ZIP_DATETIME(_year, _month, _day, _hours, _minutes, _seconds) ( (((uint32_t)((_year) - 1980) & 0x7f) << 25) | (((uint32_t)(_month) & 0x0f) << 21) | (((uint32_t)(_day) & 0x1f) << 16) | (((uint32_t)(_hours) & 0x1f) << 11) | (((uint32_t)(_minutes) & 0x3f) << 5) | (((uint32_t)(_seconds) & 0x3f) >> 1))


// File state tracking
typedef struct zipwriter_file_tag_t
{
//private:
	const char *filename;				// pointer to filename must still be valid when central directory entry is written
	uint32_t offset;				// start of file offset
	uint32_t modified;				// modified date, create with ZIP_DATETIME()
	uint32_t length;				// file length (bytes)
	uint16_t extraFieldLength;		// extra field length can be used for alignment
	uint32_t crc;					// ZIP CRC
	struct zipwriter_file_tag_t *next;	// next element in linked list
} zipwriter_file_t;

// ZIP writer state tracking
typedef struct
{
//private:
	uint32_t length;					// overall ZIP file length
	int numFiles;						// number of files in the archive
	zipwriter_file_t *files;			// head of linked list of files
	zipwriter_file_t *lastFile;			// tail of linked list of files
	zipwriter_file_t *currentFile;		// current file being written
	zipwriter_file_t *centralDirectoryFile;	// current central directory file entry being written
	int centralDirectoryEntries;
	uint32_t centralDirectoryOffset;
	uint32_t centralDirectorySize;
} zipwriter_t;


// Initialize a ZIP Writer context
void ZIPWriterInitialize(zipwriter_t *context);

// Generate a ZIP local header for a file (before each file content), using the given file context to store state, written to the specified buffer (the filename pointer must remain valid until the central directory entry is written)
size_t ZIPWriterStartFile(zipwriter_t *context, zipwriter_file_t *file, const char *filename, uint32_t modified, size_t alignment, void *buffer);

// Update the ZIP Writer's context based on the current ZIP file's data (updates length and CRCs)
void ZIPWriterFileContent(zipwriter_t *context, const void *data, size_t length);

// Generate the ZIP local header for the current file (after file content), written to the specified buffer.
size_t ZIPWriterEndFile(zipwriter_t *context, void *buffer);

// Generate a ZIP central directory entry (call once for each file, returns 0 when none remaining), written to the specified buffer.
size_t ZIPWriterCentralDirectoryEntry(zipwriter_t *context, void *buffer);

// Generate the ZIP central directory end (after central directory entries), written to the specified buffer.
size_t ZIPWriterCentralDirectoryEnd(zipwriter_t *context, void *buffer);

#ifdef __cplusplus
}
#endif

#endif
