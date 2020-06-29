// ZIP Writer (currently uncompressed)
// Dan Jackson, 2014

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
//#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zipwriter.h"


// Karl Malbrain's compact CRC-32. See "A compact CCITT crc16 and crc32 C implementation that balances processor cache usage against speed": http://www.geocities.com/malbrain/
// This implementation by Rich Geldreich <richgel99@gmail.com> from miniz.c (public domain zlib-subset - "This is free and unencumbered software released into the public domain")
#define CRC32_INIT (0)
static uint32_t crc32(uint32_t crc, const uint8_t *ptr, size_t buf_len)
{
	static const uint32_t s_crc32[16] = { 0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };
	uint32_t crcu32 = (uint32_t)crc;
	if (!ptr) return CRC32_INIT;
	crcu32 = (~crcu32) & 0xfffffffful;
	while (buf_len--)
	{
		uint8_t b = *ptr++;
		crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
		crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
	}
	return (~crcu32) & 0xfffffffful;
}


void ZIPWriterInitialize(zipwriter_t *context)
{
	// Clear context
	memset(context, 0, sizeof(zipwriter_t));
}

// Generate the ZIP local header for a file
size_t ZIPWriterStartFile(zipwriter_t *context, zipwriter_file_t *file, const char *filename, uint32_t modified, size_t alignment, void *buffer)
{
	// Start writing header
	uint8_t *p = (uint8_t *)buffer;

	// Initialize file structure
	memset(file, 0, sizeof(zipwriter_file_t));
	file->filename = filename;		// TODO: Check length
	file->modified = modified;
	file->offset = context->length;
	file->extraFieldLength = 0;

	// Calculate extra field length to match alignment
	if (alignment > 0)
	{
		uint32_t contentOffset = file->offset + 30 + strlen(file->filename);
		uint32_t alignedOffset = (contentOffset + alignment - 1) / alignment * alignment;
		if (contentOffset != alignedOffset)
		{
			file->extraFieldLength = (uint16_t)(alignedOffset - contentOffset);
		}
	}

	// Start file list, or append if we've already got one
	if (context->centralDirectoryFile == NULL) { context->centralDirectoryFile = file; }
	if (context->files == NULL)
	{
		context->files = file;
	}
	else
	{
		context->lastFile->next = file;
	}

	// Update file pointer
	context->currentFile = file;
	context->lastFile = file;
	context->numFiles++;

	// Local header
	p[0] = 0x50; p[1] = 0x4b; p[2] = 0x03; p[3] = 0x04; // Local file header signature
	p[4] = 0x14; p[5] = 0x00;					// Version needed to extract
	p[6] = 0x00 | (1 << 3); p[7] = 0x00;			// Flags (b3 = data descriptor)
	p[8] = 0; p[9] = 0;							// Compression method (0=store, 8=deflated)
	p[10] = (uint8_t)(context->currentFile->modified); p[11] = (uint8_t)(context->currentFile->modified >> 8);					// Modification time
	p[12] = (uint8_t)(context->currentFile->modified >> 16); p[13] = (uint8_t)(context->currentFile->modified >> 24);			// Modification date
	p[14] = 0; p[15] = 0; p[16] = 0; p[17] = 0;	// CRC32
	p[18] = 0; p[19] = 0; p[20] = 0; p[21] = 0;	// Compressed size
	p[22] = 0; p[23] = 0; p[24] = 0; p[25] = 0;	// Uncompressed size
	p[26] = (uint8_t)strlen(context->currentFile->filename); p[27] = (uint8_t)(strlen(context->currentFile->filename) >> 8);		// Filename length
	p[28] = (uint8_t)(context->currentFile->extraFieldLength); p[29] = (uint8_t)(context->currentFile->extraFieldLength >> 8);	// Extra field length
	memcpy(p + 30, context->currentFile->filename, strlen(context->currentFile->filename));
	p += 30 + strlen(context->currentFile->filename);

	// Extra field (padding)
	if (file->extraFieldLength > 0)
	{
		memset(p, 0, file->extraFieldLength);
		p += file->extraFieldLength;
	}

	context->currentFile->crc = CRC32_INIT;

	context->length += (int)((char *)p - (char *)buffer);
	return (int)((char *)p - (char *)buffer);
}


// Update the context with the ZIP file data
void ZIPWriterFileContent(zipwriter_t *context, const void *data, size_t length)
{
	// Update CRC
	context->currentFile->crc = crc32(context->currentFile->crc, data, length);

	// Update file and archive lengths
	context->currentFile->length += length;
	context->length += length;
}


// Generate the ZIP local header for a file
size_t ZIPWriterEndFile(zipwriter_t *context, void *buffer)
{
	// Start writing header
	uint8_t *p = (uint8_t *)buffer;

	// Extended local header
	p[0] = 0x50; p[1] = 0x4b; p[2] = 0x07; p[3] = 0x08; // Extended local file header signature
	p[4] = (uint8_t)(context->currentFile->crc); p[5] = (uint8_t)(context->currentFile->crc >> 8); p[6] = (uint8_t)(context->currentFile->crc >> 16); p[7] = (uint8_t)(context->currentFile->crc >> 24);	// CRC32
	p[8] = (uint8_t)(context->currentFile->length); p[9] = (uint8_t)(context->currentFile->length >> 8); p[10] = (uint8_t)(context->currentFile->length >> 16); p[11] = (uint8_t)(context->currentFile->length >> 24);	// Compressed size
	p[12] = (uint8_t)(context->currentFile->length); p[13] = (uint8_t)(context->currentFile->length >> 8); p[14] = (uint8_t)(context->currentFile->length >> 16); p[15] = (uint8_t)(context->currentFile->length >> 24);	// Uncompressed size
	p += 16;

	context->length += (int)((char *)p - (char *)buffer);
	return (int)((char *)p - (char *)buffer);
}


// Generate a ZIP central directory entry
size_t ZIPWriterCentralDirectoryEntry(zipwriter_t *context, void *buffer)
{
	// Start writing header
	uint8_t *p = (uint8_t *)buffer;

	// Is this the start of the central directory?
	if (context->centralDirectoryEntries <= 0)
	{
		context->centralDirectoryOffset = context->length;
		context->centralDirectorySize = 0;
		//context->centralDirectoryFile = context->files;
	}

	// Are there any remaining entries?
	if (context->centralDirectoryEntries >= context->numFiles || context->centralDirectoryFile == NULL)
	{
		return 0;
	}

	// Starting this entry
	context->centralDirectoryEntries++;

	// Central directory
	p[0] = 0x50; p[1] = 0x4b; p[2] = 0x01; p[3] = 0x02; // Central directory
	p[4] = 0x14; p[5] = 0x00;					// Version made by
	p[6] = 0x14; p[7] = 0x00;					// Version needed to extract
	p[8] = (1 << 3); p[9] = 0x00;				// General purpose bit flag
	p[10] = 0; p[11] = 0;						// Compression method (0=store, 8=deflated)
	p[12] = (uint8_t)(context->centralDirectoryFile->modified); p[13] = (uint8_t)(context->centralDirectoryFile->modified >> 8);					// Modification time
	p[14] = (uint8_t)(context->centralDirectoryFile->modified >> 16); p[15] = (uint8_t)(context->centralDirectoryFile->modified >> 24);			// Modification date
	p[16] = (uint8_t)(context->centralDirectoryFile->crc); p[17] = (uint8_t)(context->centralDirectoryFile->crc >> 8); p[18] = (uint8_t)(context->centralDirectoryFile->crc >> 16); p[19] = (uint8_t)(context->centralDirectoryFile->crc >> 24);	// CRC32
	p[20] = (uint8_t)(context->centralDirectoryFile->length); p[21] = (uint8_t)(context->centralDirectoryFile->length >> 8); p[22] = (uint8_t)(context->centralDirectoryFile->length >> 16); p[23] = (uint8_t)(context->centralDirectoryFile->length >> 24);	// Compressed size
	p[24] = (uint8_t)(context->centralDirectoryFile->length); p[25] = (uint8_t)(context->centralDirectoryFile->length >> 8); p[26] = (uint8_t)(context->centralDirectoryFile->length >> 16); p[27] = (uint8_t)(context->centralDirectoryFile->length >> 24);	// Uncompressed size
	p[28] = (uint8_t)strlen(context->centralDirectoryFile->filename); p[29] = (uint8_t)(strlen(context->centralDirectoryFile->filename) >> 8);	// Filename length
	p[30] = (uint8_t)(context->centralDirectoryFile->extraFieldLength); p[31] = (uint8_t)(context->centralDirectoryFile->extraFieldLength >> 8);// Extra field length
	p[32] = 0; p[33] = 0;						// File comment length
	p[34] = 0; p[35] = 0;						// Disk number start
	p[36] = 0; p[37] = 0;						// Internal file attributes
	p[38] = 0; p[39] = 0; p[40] = 0; p[41] = 0;	// External file attributes
	p[42] = (uint8_t)(context->centralDirectoryFile->offset); p[43] = (uint8_t)(context->centralDirectoryFile->offset >> 8); p[44] = (uint8_t)(context->centralDirectoryFile->offset >> 16); p[45] = (uint8_t)(context->centralDirectoryFile->offset >> 24);	// Relative offset of local header
	memcpy(p + 46, context->centralDirectoryFile->filename, strlen(context->centralDirectoryFile->filename));
	p += 46 + strlen(context->centralDirectoryFile->filename);

	// Extra field (padding)
	if (context->centralDirectoryFile->extraFieldLength > 0)
	{
		memset(p, 0, context->centralDirectoryFile->extraFieldLength);
		p += context->centralDirectoryFile->extraFieldLength;
	}

	// Advance to the next entry
	context->centralDirectoryFile = context->centralDirectoryFile->next;

	context->centralDirectorySize += (int)((char *)p - (char *)buffer);
	context->length += (int)((char *)p - (char *)buffer);
	return (int)((char *)p - (char *)buffer);
}


// Generate the ZIP central directory end
size_t ZIPWriterCentralDirectoryEnd(zipwriter_t *context, void *buffer)
{
	// Start writing header
	uint8_t *p = (uint8_t *)buffer;

	// Local header
	p[0] = 0x50; p[1] = 0x4b; p[2] = 0x05; p[3] = 0x06; // End of central directory header
	p[4] = 0x00; p[5] = 0x00;					// Number of this disk
	p[6] = 0x00; p[7] = 0x00;					// Number of the disk with the start of the central directory
	p[8] = context->numFiles; p[9] = 0;			// Number of entries in the central directory on this disk
	p[10] = context->numFiles; p[11] = 0;		// Total number of entries in the central directory
	
	p[12] = (uint8_t)(context->centralDirectorySize); p[13] = (uint8_t)(context->centralDirectorySize >> 8); p[14] = (uint8_t)(context->centralDirectorySize >> 16); p[15] = (uint8_t)(context->centralDirectorySize >> 24);			// Size of the central directory
	p[16] = (uint8_t)(context->centralDirectoryOffset); p[17] = (uint8_t)(context->centralDirectoryOffset >> 8); p[18] = (uint8_t)(context->centralDirectoryOffset >> 16); p[19] = (uint8_t)(context->centralDirectoryOffset >> 24);	// Offset of the start of the central directory from the starting disk
	p[20] = 0; p[21] = 0;						// ZIP file comment length
	p += 22;

	context->length += (int)((char *)p - (char *)buffer);
	return (int)((char *)p - (char *)buffer);
}

