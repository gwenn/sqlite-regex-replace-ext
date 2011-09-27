/*
 * Written by Alexey Tourbin <at@altlinux.org>.
 * Adapted from pcre matching to glib regex_replace.
 *
 * The author has dedicated the code to the public domain.  Anyone is free
 * to copy, modify, publish, use, compile, sell, or distribute the original
 * code, either in source code form or as a compiled binary, for any purpose,
 * commercial or non-commercial, and by any means.
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1

typedef struct {
	char *s;
	GRegex *p;
} cache_entry;

#ifndef CACHE_SIZE
#define CACHE_SIZE 16
#endif

static void regex_replace(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
	const gchar *re, *str, *replacement;
	GRegex *p;

	assert(argc == 3);

	re = (const gchar *) sqlite3_value_text(argv[0]);
	if (!re) {
		sqlite3_result_error(ctx, "no regexp", -1);
		return;
	}

	str = (const gchar *) sqlite3_value_text(argv[1]);
	if (!str) {
		sqlite3_result_error(ctx, "no string", -1);
		return;
	}

	replacement = (const gchar *) sqlite3_value_text(argv[2]);
	if (!replacement) {
		sqlite3_result_error(ctx, "no replacement string", -1);
		return;
	}

	/* simple LRU cache */
	{
		int i;
		int found = 0;
		cache_entry *cache = sqlite3_user_data(ctx);

		assert(cache);

		for (i = 0; i < CACHE_SIZE && cache[i].s; i++)
			if (strcmp(re, cache[i].s) == 0) {
				found = 1;
				break;
			}
		if (found) {
			if (i > 0) {
				cache_entry c = cache[i];
				memmove(cache + 1, cache, i * sizeof(cache_entry));
				cache[0] = c;
			}
		} else {
			cache_entry c;
			GError *err = NULL;
			c.p = g_regex_new(re, 0, 0, &err);
			if (!c.p) {
				char *e2 = sqlite3_mprintf("%s: %s", re, err->message);
				sqlite3_result_error(ctx, e2, -1);
				sqlite3_free(e2);
				g_error_free(err);
				return;
			}
			c.s = strdup(re);
			if (!c.s) {
				sqlite3_result_error(ctx, "strdup: ENOMEM", -1);
				g_regex_unref(c.p);
				return;
			}
			i = CACHE_SIZE - 1;
			if (cache[i].s) {
				free(cache[i].s);
				assert(cache[i].p);
				g_regex_unref(cache[i].p);
			}
			memmove(cache + 1, cache, i * sizeof(cache_entry));
			cache[0] = c;
		}
		p = cache[0].p;
	}

	{
		GError *err = NULL;
		gchar *result = NULL;
		assert(p);
		result = g_regex_replace(p, str, -1, 0, replacement, 0, &err);
		if (err) {
			sqlite3_result_error(ctx, err->message, -1);
			g_error_free(err);
			return;
		}
		sqlite3_result_text(ctx, result, -1, g_free);
		return;
	}
}

int sqlite3_extension_init(sqlite3 *db, char **err, const sqlite3_api_routines *api) {
	SQLITE_EXTENSION_INIT2(api)
		cache_entry *cache = calloc(CACHE_SIZE, sizeof(cache_entry));
	if (!cache) {
		*err = "calloc: ENOMEM";
		return 1;
	}
	sqlite3_create_function(db, "regex_replace", 3, SQLITE_UTF8, cache, regex_replace, NULL, NULL);
	return 0;
}
