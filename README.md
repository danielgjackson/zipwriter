# ZIP Writer

Minimal, low-resource .ZIP file writer (store only), suitable for embedded uses.  The API allows streaming of the output where each file's length does not need to be known in advance.

Required files: [`zipwriter.h`](zipwriter.h) [`zipwriter.c`](zipwriter.c)

You will need a buffer of at least `ZIP_WRITER_SIZE_MAX` bytes, and a variable (`len`) to track the number of bytes to write:

```c
unsigned char buffer[ZIP_WRITER_SIZE_MAX];
size_t len;
```

To write a .ZIP file you will need a context object and to initialize it:

```c
zipwriter_t zip;
ZIPWriterInitialize(&zip);
```

Then, for each file you would like to store in the .ZIP file, you will need a file context and to write the start of a ZIP _local header_ for the file (the filename pointer must remain valid, and the contents unchanged until after the central directory entry has been written):

```c
zipwriter_file_t file1;
len = ZIPWriterStartFile(&zip, &file1, "MYFILE.TXT", ZIP_DATETIME(1999,12,31,23,59,59), 0, buffer);
// TODO: Write `len` bytes from `buffer` to the .ZIP file.
```

Now write the contents of your file and, for any chunks of bytes that you write, also call this function to update the context based on the file data (this internally updates the length and CRC):

```c
ZIPWriterFileContent(&zip, writtenData, writtenLength);
// TODO: Write `writtenLength` bytes from `writtenData` (your file contents) to the .ZIP file.
```

...and after each file, write the end of the .ZIP local header:

```c
len = ZIPWriterEndFile(&zip, buffer);
// TODO: Write `len` bytes from `buffer` to the .ZIP file.
```

After you have written all of your files, you will need to write the .ZIP file's "central directory".  This needs to be called multiple times (once for each file), and will return `0` when none remaining:

```c
while ((len = ZIPWriterCentralDirectoryEntry(&zip, buffer)) != 0) {
    // TODO: Write `len` bytes from `buffer` to the .ZIP file.
}
```

Finally, write the end of the central directory:

```c
len = ZIPWriterCentralDirectoryEnd(&zip, buffer);
// TODO: Write `len` bytes from `buffer` to the .ZIP file.
```
