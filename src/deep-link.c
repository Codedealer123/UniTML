#include "deep_link.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *parse_deep_link(const char *url) {
    if (!url || strncmp(url, "unitml://", 9) != 0) {
        return NULL;
    }

    // JSON-escape the URL minimally (quotes and backslashes)
    size_t ulen = strlen(url);
    size_t maxlen = ulen * 2 + 1; // worst-case if every char is escaped
    char *escaped = (char *)malloc(maxlen);
    if (!escaped) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < ulen; i++) {
        char c = url[i];
        if (c == '"' || c == '\\') {
            if (j + 2 >= maxlen) { free(escaped); return NULL; }
            escaped[j++] = '\\';
            escaped[j++] = c;
        } else {
            if (j + 1 >= maxlen) { free(escaped); return NULL; }
            escaped[j++] = c;
        }
    }
    escaped[j] = '\0';

    size_t len = j + 64;
    char *json = (char *)malloc(len);
    if (!json) { free(escaped); return NULL; }

    snprintf(json, len, "{\"protocol\":\"unitml\",\"url\":\"%s\"}", escaped);
    free(escaped);
    return json;
}
