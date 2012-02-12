/*
 * Written by Alexey Tourbin <at@altlinux.org>.
 * Adapted from pcre matching to glib regex_replace.
 *
 * The author has dedicated the code to the public domain.  Anyone is free
 * to copy, modify, publish, use, compile, sell, or distribute the original
 * code, either in source code form or as a compiled binary, for any purpose,
 * commercial or non-commercial, and by any means.
 */
#include <glib.h>

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1

static void glibRegexpDelete(void *p){
  GRegex *pRegex = (GRegex *)p;
  g_regex_unref(pRegex);
}

static void glibReplaceAllFunc(
  sqlite3_context *ctx,
  int argc,
  sqlite3_value **argv
){
	GError *err = NULL;
	GRegex *p;
	gchar *result = NULL;

	(void)argc;  /* Unused parameter */

	const gchar *str = (const gchar *) sqlite3_value_text(argv[1]);
	if (!str) {
		return;
	}

	const gchar *replacement = (const gchar *) sqlite3_value_text(argv[2]);
	if (!replacement) {
		sqlite3_result_error(ctx, "no replacement string", -1);
		return;
	}

	p = sqlite3_get_auxdata(ctx, 0);
	if( !p ){
		const gchar *re = (const gchar *) sqlite3_value_text(argv[0]);
		if( !re ){
			//sqlite3_result_error(ctx, "no regexp", -1);
			return;
		}
		p = g_regex_new(re, 0, 0, &err);

		if( p ){
			sqlite3_set_auxdata(ctx, 0, p, glibRegexpDelete);
		}else{
			char *e2 = sqlite3_mprintf("%s: %s", re, err->message);
			sqlite3_result_error(ctx, e2, -1);
			sqlite3_free(e2);
			g_error_free(err);
			return;
		}
	}

	result = g_regex_replace(p, str, -1, 0, replacement, 0, &err);
	if (err) {
		sqlite3_result_error(ctx, err->message, -1);
		g_error_free(err);
		return;
	}
	sqlite3_result_text(ctx, result, -1, g_free);
}

int sqlite3_extension_init(
  sqlite3 *db,
  char **err,
  const sqlite3_api_routines *api
) {
	SQLITE_EXTENSION_INIT2(api)
	sqlite3_create_function_v2(db, "regex_replace", 3, SQLITE_UTF8, 0, glibReplaceAllFunc, NULL, NULL, NULL);
	return 0;
}
