#ifndef DB_H
#define DB_H

#include <sqlite3.h>

//Schema + pragmas
int db_init(sqlite3 *db);

int exec_sql(sqlite3 *db, const char *sql);

#endif