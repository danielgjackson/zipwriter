// ZIP Writer Test Program
// Dan Jackson, 2014

// [
//   ZIP LOCAL HEADER
//   FILE CONTENT
//   ZIP EXTENDED LOCAL HEADER
// ]...
// ZIP CENTRAL DIRECTORY ENTRY...
// ZIP END CENTRAL DIRECTORY

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#define strdup _strdup
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zipwriter.h"

#define FILE_ALIGNMENT 0		// File contents starts at a specific alignment within the zip file
#define NUM_FILES 3

int main(int argc, char *argv[])
{
	char *filename = "test.zip";
	zipwriter_t zip;
	zipwriter_file_t files[NUM_FILES];
	FILE *fp;
	unsigned char buffer[ZIP_WRITER_SIZE_MAX + FILE_ALIGNMENT];
	int len;

#ifdef _DEBUG
	atexit(getchar);
#endif

	// Start writing
	fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		fprintf(stderr, "ERROR: Problem opening file: %s\n", filename);
		return -1;
	}
	ZIPWriterInitialize(&zip);

	// Write files
	for (int i = 0; i < NUM_FILES; i++)
	{
		char filename[32];
		char content[512];

		sprintf(filename, "FILE%04d.TXT", i + 1);
		memset(content, 0, sizeof(content));
		sprintf(content, "FILE CONTENTS %d!", i + 1);

		// Write header
		len = ZIPWriterStartFile(&zip, &files[i], strdup(filename), ZIP_DATETIME(2000,1,1,0,0,0), FILE_ALIGNMENT, buffer);
		fwrite(buffer, 1, len, fp);

		// Update for content
		len = sizeof(content);  // sizeof(content); // strlen(content);
		ZIPWriterFileContent(&zip, content, len);
		fwrite(content, 1, len, fp);

		// Write footer
		len = ZIPWriterEndFile(&zip, buffer);
		fwrite(buffer, 1, len, fp);
	}
	
	// Write central directory entries
	while ((len = ZIPWriterCentralDirectoryEntry(&zip, buffer)) > 0)
	{
		fwrite(buffer, 1, len, fp);
	}

	// No more central directory entries
	len = ZIPWriterCentralDirectoryEnd(&zip, buffer);
	fwrite(buffer, 1, len, fp);

	// Finished .ZIP file
	fclose(fp);

	// Free _strdup()'d filenames
	for (int i = 0; i < NUM_FILES; i++) free((char *)files[i].filename);

	return 0;
}

