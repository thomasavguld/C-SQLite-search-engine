#ifndef FS_H
#define FS_H

#include <sqlite3.h>

typedef void (*file_callback) (const char *filepath, void *userdata);

char *read_file(const char *filepath);
void list_files(const char *path, file_callback cb, void *userdata); 

#endif



