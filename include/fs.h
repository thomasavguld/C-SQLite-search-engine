#ifndef FS_H
#define FS_H

#include <stddef.h>

typedef void (*file_callback)(const char *path, void *userdata);

char *read_file(const char *path);

void list_files(const char *root, file_callback cb, void *userdata);

#endif