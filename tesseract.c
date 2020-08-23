#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <leptonica/allheaders.h>
#include <tesseract/capi.h>

static char *get_text(const char *file, const char *lang) {
    char *str=NULL;
    TessBaseAPI *handle;
    PIX *img;
    char *text;

    if((img = pixRead(file)) == NULL)
        return NULL;

    handle = TessBaseAPICreate();
    if(TessBaseAPIInit3(handle, NULL, lang) != 0)
        return NULL;

    TessBaseAPISetImage2(handle, img);
    if(TessBaseAPIRecognize(handle, NULL) != 0)
        return NULL;

    if((text = TessBaseAPIGetUTF8Text(handle)) == NULL)
        return NULL;

    str=strdup(text);

    TessDeleteText(text);
    TessBaseAPIEnd(handle);
    TessBaseAPIDelete(handle);
    pixDestroy(&img);

    return str;
}


static void tesseract_read_func(
    sqlite3_context *context,
    int argc,
    sqlite3_value **argv
) {
    assert( argc==2 );
    if(sqlite3_value_type(argv[0])!=SQLITE_TEXT )
        return;

    const char *file=(const char *) sqlite3_value_text(argv[0]);
    const char *lang=	sqlite3_value_type(argv[1])==SQLITE_TEXT ? 
						(const char *) sqlite3_value_text(argv[1]) : "eng";

    if(access(file, F_OK) == -1) {
        sqlite3_result_null(context);
        return;
    }

    char *str;
    if((str=get_text(file, lang))==NULL) {
        sqlite3_result_null(context);
        return;
    }

    sqlite3_result_text(context, str, -1, free);
}

#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_tesseract_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi
) {
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
    (void)pzErrMsg;  /* Unused parameter */
    rc = sqlite3_create_function(db, "tesseract_read", 2,
#if SQLITE_VERSION_NUMBER > 3031000
                                 SQLITE_UTF8|SQLITE_INNOCUOUS|SQLITE_DETERMINISTIC,
#else
                                 SQLITE_UTF8|SQLITE_DETERMINISTIC,
#endif
                                 0, tesseract_read_func, 0, 0);
    return rc;
}

