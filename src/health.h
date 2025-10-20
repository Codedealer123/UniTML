#ifndef HEALTH_H
#define HEALTH_H

/* Wait for HTTP endpoint to return 200 OK. Returns 0 on success, -1 on timeout/error. */
int wait_for_health(const char *url, int timeout_ms);

#endif