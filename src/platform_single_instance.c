#include "platform_single_instance.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static HANDLE g_mutex = NULL;

int acquire_single_instance(void) {
    g_mutex = CreateMutexA(NULL, TRUE, "Global\\unitml-singleton");
    if (g_mutex == NULL) {
        return -1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(g_mutex);
        g_mutex = NULL;
        return -1;
    }
    return 0;
}

void release_single_instance(void) {
    if (g_mutex) {
        ReleaseMutex(g_mutex);
        CloseHandle(g_mutex);
        g_mutex = NULL;
    }
}

#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>

static int g_lockfd = -1;

int acquire_single_instance(void) {
    const char *lockpath = "/tmp/unitml.lock";
    
    char *home = getenv("HOME");
    char cachepath[512];
    if (home) {
        snprintf(cachepath, sizeof(cachepath), "%s/.cache/unitml", home);
        mkdir(cachepath, 0755);
        snprintf(cachepath, sizeof(cachepath), "%s/.cache/unitml/lock", home);
        lockpath = cachepath;
    }
    
    g_lockfd = open(lockpath, O_CREAT | O_RDWR, 0600);
    if (g_lockfd < 0) {
        return -1;
    }
    
    if (flock(g_lockfd, LOCK_EX | LOCK_NB) != 0) {
        close(g_lockfd);
        g_lockfd = -1;
        return -1;
    }
    
    return 0;
}

void release_single_instance(void) {
    if (g_lockfd >= 0) {
        flock(g_lockfd, LOCK_UN);
        close(g_lockfd);
        g_lockfd = -1;
    }
}
#endif