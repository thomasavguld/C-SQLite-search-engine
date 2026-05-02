#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

#include "fs.h"


// List files

void list_files(const char *path, file_callback cb, void *userdata) { 

	DIR *dir = opendir(path);
	if (!dir) {
		perror("Opendir failed.\n");
		printf("Dir: %s\n", path);
		return;
	}

	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL) {
		
//		printf("File: %s/%s\n", path, entry->d_name);

		if (entry->d_name[0] == '.') continue;
		
		char fullpath[PATH_MAX];
		snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

		const char *ext = strrchr(entry->d_name, '.');
		if (!ext || strcmp(ext, ".json") !=0)
			continue;

		cb(fullpath, userdata);
	}

	closedir(dir);

}

char *read_file(const char *filepath) {
	FILE *file = fopen(filepath, "r");
	if (!file) return NULL;

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);
	
	if (size <=0) {
		fclose(file);
		return NULL;
	}

	char *buffer = malloc(size + 1);	
	if (!buffer) {
		fclose(file);
		return NULL;
	}

	size_t read = fread(buffer, 1, size, file);
	if (read != size) {
		free(buffer);
		fclose(file);
		return NULL;
	}

	buffer[size] = '\0';
	fclose(file);
	return buffer;
}
