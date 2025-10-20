#ifndef DEEP_LINK_H
#define DEEP_LINK_H

/* Parse deep link URL and return JSON string for JS dispatch. Caller must free. */
char *parse_deep_link(const char *url);

#endif