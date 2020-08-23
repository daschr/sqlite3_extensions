#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
static int dayofweek(int y, int m, int d) {
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

static int is_digit(const char *s) {
    if(s[0]=='\0')
        return 0;

    for(int i=0; s[i]!='\0'; ++i)
        if(s[i] < '0' || s[i] > '9')
            return 0;

    return 1;
}


static int compare_single(unsigned int v, const char *e) {
    if(*e == '\0')
        return 0;

    if(e[0] == '*') {
        if(e[1] == '\0')
            return 1;
        else if(e[1] == '/') {
            if(!is_digit(e+2))
                return 0;
            else
                return !(v % atoi(e+2));
        }
    }

    int epos=0, cv;
    while(e[epos]!='\0') {
        int i=0;
        for(; e[epos+i]>='0' && e[epos+i]<='9'; ++i);

        if(i==0) return 0;

        if(sscanf(e+epos,"%d*",&cv) != 1)
            return 0;

        if(cv==v)
            return 1;

        epos+=++i;
    }

    return 0;
}

static int compare(const char *date, char *cronentry) {
    unsigned int d[5];

    if(sscanf(date, "%u-%u-%u %u:%u:*", &d[0], &d[1], &d[2], &d[3], &d[4]) != 5)
        return 0;

    int pos=0;
    const char *s=strtok(cronentry," ");

    while(s != NULL && pos < 4) {
        if(!compare_single(d[4-pos++], s))
            return 0;
        s=strtok(NULL, " ");
    }

    if(s== NULL || pos != 4)
        return 0;

    return compare_single(dayofweek(d[0], d[1], d[2]), s);
}

/*
# ┌───────────── minute (0 - 59)
# │ ┌───────────── hour (0 - 23)
# │ │ ┌───────────── day of the month (1 - 31)
# │ │ │ ┌───────────── month (1 - 12)
# │ │ │ │ ┌───────────── day of the week (0 - 6) (Sunday to Saturday;
# │ │ │ │ │                                   7 is also Sunday on some systems)
# │ │ │ │ │
# │ │ │ │ │
# * * * * * <command to execute>
*/

static void cron_match_func(
    sqlite3_context *context,
    int argc,
    sqlite3_value **argv
) {
    const char *date;
    char *cronentry;

    assert( argc==2 );
    if( sqlite3_value_type(argv[0])==SQLITE_NULL ||
            sqlite3_value_type(argv[1])==SQLITE_NULL )
        return;

    date=(const char *) sqlite3_value_text(argv[0]);

    sqlite3_value *cpy=sqlite3_value_dup(argv[1]);
    cronentry=(char *) sqlite3_value_text(cpy);

    sqlite3_result_int(context, compare(date, cronentry));

    sqlite3_value_free(cpy);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_cron_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi
) {
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
    (void)pzErrMsg;  /* Unused parameter */
    rc = sqlite3_create_function(db, "cron_match", 2,
#if SQLITE_VERSION_NUMBER > 3031000
                                 SQLITE_UTF8|SQLITE_INNOCUOUS|SQLITE_DETERMINISTIC,
#else
                                 SQLITE_UTF8|SQLITE_DETERMINISTIC,
#endif
                                 0, cron_match_func, 0, 0);
    return rc;
}

