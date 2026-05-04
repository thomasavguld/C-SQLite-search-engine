#ifndef DB_H
#define DB_H

#include <sqlite3.h>

/* schema + pragmas */
int db_init(sqlite3 *db);

/* optional utility (kan tas bort helt om du vill) */
int exec_sql(sqlite3 *db, const char *sql);

#endif