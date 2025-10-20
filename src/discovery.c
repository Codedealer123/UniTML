#include "discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <sys/stat.h>
#define PATH_SEP '\\'
#define STAT_FN _stat
#define STAT_T struct _stat
#define IS_DIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#define IS_REG(m) (((m) & _S_IFMT) == _S_IFREG)
#else
#define PATH_SEP '/'
#define STAT_FN stat
#define STAT_T struct stat
#define IS_DIR(m) S_ISDIR(m)
#define IS_REG(m) S_ISREG(m)
#endif

static int file_exists(const char *path) {
    STAT_T st;
    return STAT_FN(path, &st) == 0 && IS_REG(st.st_mode);
}

static int dir_exists(const char *path) {
    STAT_T st;
    return STAT_FN(path, &st) == 0 && IS_DIR(st.st_mode);
}

static int find_backend(const char *exe_dir, discovery_result_t *result) {
    const char *backend_dirs[] = {"backend", "server", "api"};
    
    for (int i = 0; i < 3; i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s%c%s", exe_dir, PATH_SEP, backend_dirs[i]);
        
        if (dir_exists(path)) {
            snprintf(result->backend_dir, sizeof(result->backend_dir), "%s", path);
            
            char pkgjson[512];
            snprintf(pkgjson, sizeof(pkgjson), "%s%cpackage.json", path, PATH_SEP);
            
            if (file_exists(pkgjson)) {
                FILE *f = fopen(pkgjson, "r");
                if (f) {
                    char line[256];
                    int has_start = 0;
                    while (fgets(line, sizeof(line), f)) {
                        if (strstr(line, "\"start\"")) {
                            has_start = 1;
                            break;
                        }
                    }
                    fclose(f);
                    
                    if (has_start) {
                        snprintf(result->backend_cmd, sizeof(result->backend_cmd), "npm run start");
                        result->has_backend = 1;
                        return 1;
                    }
                }
            }
            
            char serverjs[512];
            snprintf(serverjs, sizeof(serverjs), "%s%cserver.js", path, PATH_SEP);
            if (file_exists(serverjs)) {
                snprintf(result->backend_cmd, sizeof(result->backend_cmd), "node server.js");
                result->has_backend = 1;
                return 1;
            }
        }
    }
    
    return 0;
}

static int find_frontend(const char *exe_dir, discovery_result_t *result) {
    const char *frontend_paths[] = {
        "frontend/dist/index.html",
        "frontend/public/index.html",
        "frontend/index.html",
        "public/index.html",
        "index.html"
    };
    
    for (int i = 0; i < 5; i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s%c%s", exe_dir, PATH_SEP, frontend_paths[i]);
        
        if (file_exists(path)) {
            snprintf(result->frontend_file, sizeof(result->frontend_file), "%s", path);
            result->has_frontend = 1;
            return 1;
        }
    }
    
    return 0;
}

int discover_app(const char *exe_dir, discovery_result_t *result) {
    memset(result, 0, sizeof(*result));
    
    char *port_env = getenv("PORT");
    result->port = port_env ? atoi(port_env) : 5173;
    if (result->port <= 0) result->port = 5173;
    
    find_backend(exe_dir, result);
    find_frontend(exe_dir, result);
    
    return (result->has_backend || result->has_frontend) ? 0 : -1;
}
