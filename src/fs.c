#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "db.h"
#include "fs.h"
#include "json.h"


// List files

void list_files(const char *path, sqlite3 *db, sqlite3_stmt *stmt_main, sqlite3_stmt *stmt_fts) { 

	(void)db; // <--- KEEP AN EYE ON THIS FOR LATER

	struct dirent *entry;
	DIR *dir = opendir(path);

	if (!dir) {
		perror("Opendir failed.\n");
		printf("Dir: %s\n", path);
		return;
	}

	while ((entry = readdir(dir)) != NULL) {

		if (entry->d_name[0] == '.') continue;
		
		size_t len = strlen(path) + strlen(entry->d_name) + 2;

		char *filepath = malloc(len);
		if (!filepath) return;

		snprintf(filepath, len, "%s/%s", path, entry->d_name);

		char *json = read_file(filepath);
		if (!json) {
			free(filepath);
			continue;
		}


		char *title = extract_title(json);
		char *abstract = extract_abstract(json);

		insert_document(stmt_main, title, abstract);
		insert_document(stmt_fts, title, abstract);
		
		free(filepath);
		free(json);
		free(title);
		free(abstract);

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
