#ifndef DISCOVERY_H
#define DISCOVERY_H

typedef struct {
    char backend_dir[512];
    char backend_cmd[1024];
    char frontend_file[512];
    int has_backend;
    int has_frontend;
    int port;
} discovery_result_t;

/* Discover backend and frontend from exe directory. Returns 0 on success. */
int discover_app(const char *exe_dir, discovery_result_t *result);

#endif