/*
** 2012 February 12
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains code for SQLite extension that uses icu to provide a regex_replace() function.
*/
#include <assert.h>

/* Include ICU headers */
#include <unicode/utypes.h>
#include <unicode/uregex.h>
#include <unicode/ustring.h>

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1

/* stolen from icu standard extension */
static void xFree(void *p){
  sqlite3_free(p);
}
/* stolen from icu standard extension */
static void icuFunctionError(
  sqlite3_context *pCtx,
  const char *zName,
  UErrorCode e
){
  char zBuf[128];
  sqlite3_snprintf(128, zBuf, "ICU error: %s(): %s", zName, u_errorName(e));
  sqlite3_result_error(pCtx, zBuf, -1);
}
/* stolen from icu standard extension */
static void icuRegexpDelete(void *p){
  URegularExpression *pExpr = (URegularExpression *)p;
  uregex_close(pExpr);
}

static void icuReplaceAllFunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  UErrorCode status = U_ZERO_ERROR;
  URegularExpression *pExpr;
  UChar *zOutput, *zOld;
  int nInput;
  int nOutput;
  int32_t destLength;

  (void)argc;  /* Unused parameter */

  nInput = sqlite3_value_bytes16(argv[1]); // Would possibly invalidate zString if called after next statement.
  const UChar *zString = sqlite3_value_text16(argv[1]);

  /* If the text is NULL, then the result is also NULL. */
  if( !zString ){
    return;
  }

  const UChar *zReplacement = sqlite3_value_text16(argv[2]);
  if ( !zReplacement ) {
    sqlite3_result_error(context, "no replacement string", -1);
    return;
  }

  pExpr = sqlite3_get_auxdata(context, 0);
  if( !pExpr ){
    const UChar *zPattern = sqlite3_value_text16(argv[0]);
    if( !zPattern ){
      return;
    }
    pExpr = uregex_open(zPattern, -1, 0, 0, &status);

    if( U_SUCCESS(status) ){
      sqlite3_set_auxdata(context, 0, pExpr, icuRegexpDelete);
    }else{
      assert(!pExpr);
      icuFunctionError(context, "uregex_open", status);
      return;
    }
  }

  /* Configure the text that the regular expression operates on. */
  uregex_setText(pExpr, zString, -1, &status);
  if( !U_SUCCESS(status) ){
    icuFunctionError(context, "uregex_setText", status);
    return;
  }
/*
  int32_t   uregex_replaceAll (URegularExpression *regexp, const UChar *replacementText, int32_t replacementLength, UChar *destBuf, int32_t destCapacity, UErrorCode *status)
*/
  nOutput = nInput; // Expect output length = input length.
  zOutput = sqlite3_malloc((nOutput+1) * sizeof(UChar)); // Take one more codepoint for null-termination, just in case.
  if( !zOutput ){
    return;
  }

  /* Attempt the replace */
  destLength = uregex_replaceAll(pExpr, zReplacement, -1, zOutput, nOutput, &status);
  if( !U_SUCCESS(status) ){
    if (U_BUFFER_OVERFLOW_ERROR == status) {
      zOld = zOutput;
      zOutput = sqlite3_realloc(zOutput, (destLength+1) * sizeof(UChar)); // Again one extra codepoint for null-termination.
      if ( !zOutput ) {
        xFree(zOld);
        icuFunctionError(context, "uregex_replaceAll", status);
        return;
      }
      status = U_ZERO_ERROR;
      destLength = uregex_replaceAll(pExpr, zReplacement, -1, zOutput, destLength, &status);
    }
    if( !U_SUCCESS(status) ){
      xFree(zOutput);
      icuFunctionError(context, "uregex_replaceAll", status);
      return;
    }
  }
  zOutput[destLength] = 0; // Hard null-terminate to be sure as icu function not fully understood.

  /* Set the text that the regular expression operates on to a NULL
  ** pointer. This is not really necessary, but it is tidier than
  ** leaving the regular expression object configured with an invalid
  ** pointer after this function returns.
  */
  uregex_setText(pExpr, 0, 0, &status);

  sqlite3_result_text16(context, zOutput, -1, xFree);
}

/* SQLite invokes this routine once when it loads the extension.
** Create new functions, collating sequences, and virtual table
** modules here.  This is usually the only exported symbol in
** the shared library.
*/
int sqlite3_extension_init(
  sqlite3 *db,
  char **pzErrMsg,
  const sqlite3_api_routines *pApi
){
  SQLITE_EXTENSION_INIT2(pApi)
  sqlite3_create_function_v2(db, "regex_replace", 3, SQLITE_ANY, 0, icuReplaceAllFunc, NULL, NULL, NULL);
  return 0;
}
