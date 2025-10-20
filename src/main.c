#define WEBVIEW_IMPLEMENTATION
#include "third_party/webview.h"
#include "platform_single_instance.h"
#include "deep_link.h"
#include "health.h"
#include "discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define PATH_MAX 512
static PROCESS_INFORMATION g_backend_proc;
static int g_backend_running = 0;

static void get_exe_dir(char *buf, size_t size) {
    GetModuleFileNameA(NULL, buf, (DWORD)size);
    char *last_sep = strrchr(buf, '\\');
    if (last_sep) *last_sep = '\0';
}

static int spawn_backend(const char *cmd, const char *work_dir, int port) {
    char port_val[16];
    snprintf(port_val, sizeof(port_val), "%d", port);
    if (_putenv_s("PORT", port_val) != 0) {
        // continue anyway; child may not need PORT
    }
    
    STARTUPINFOA si = {sizeof(si)};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    char cmd_buf[1024];
    snprintf(cmd_buf, sizeof(cmd_buf), "cmd.exe /C cd /D \"%s\" && %s", work_dir, cmd);
    
    if (!CreateProcessA(NULL, cmd_buf, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP, NULL, work_dir, &si, &g_backend_proc)) {
        return -1;
    }
    g_backend_running = 1;
    return 0;
}

static void kill_backend(void) {
    if (g_backend_running) {
        GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, g_backend_proc.dwProcessId);
        if (WaitForSingleObject(g_backend_proc.hProcess, 3000) == WAIT_TIMEOUT) {
            TerminateProcess(g_backend_proc.hProcess, 1);
        }
        CloseHandle(g_backend_proc.hProcess);
        CloseHandle(g_backend_proc.hThread);
        g_backend_running = 0;
    }
}

#else
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <libgen.h>
#include <limits.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

static pid_t g_backend_pid = -1;

static void get_exe_dir(char *buf, size_t size) {
#ifdef __APPLE__
    uint32_t sz = (uint32_t)size;
    if (_NSGetExecutablePath(buf, &sz) != 0) {
        if (size > 0) buf[0] = '\0';
        return;
    }
#else
    ssize_t len = readlink("/proc/self/exe", buf, size - 1);
    if (len > 0) buf[len] = '\0';
#endif
    char *dir = dirname(buf);
    if (dir) {
        strncpy(buf, dir, size);
        buf[size - 1] = '\0';
    }
}

static int spawn_backend(const char *cmd, const char *work_dir, int port) {
    char port_str[32];
    snprintf(port_str, sizeof(port_str), "%d", port);
    setenv("PORT", port_str, 1);
    
    g_backend_pid = fork();
    if (g_backend_pid == 0) {
        chdir(work_dir);
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        _exit(1);
    }
    return g_backend_pid > 0 ? 0 : -1;
}

static void kill_backend(void) {
    if (g_backend_pid > 0) {
        kill(g_backend_pid, SIGTERM);
        int status;
        int waited = 0;
        while (waited < 3 && waitpid(g_backend_pid, &status, WNOHANG) == 0) {
            sleep(1);
            waited++;
        }
        if (waitpid(g_backend_pid, &status, WNOHANG) == 0) {
            kill(g_backend_pid, SIGKILL);
            waitpid(g_backend_pid, &status, 0);
        }
        g_backend_pid = -1;
    }
}
#endif

static void native_ping(const char *seq, const char *req, void *arg) {
    webview_t w = (webview_t)arg;
    (void)req;
    webview_return(w, seq, 0, "{\"ok\":true}");
}

int main(int argc, char *argv[]) {
    if (acquire_single_instance() != 0) {
        fprintf(stderr, "Another instance is already running.\n");
        return 1;
    }
    
    char exe_dir[PATH_MAX];
    get_exe_dir(exe_dir, sizeof(exe_dir));
    
    discovery_result_t disco;
    if (discover_app(exe_dir, &disco) != 0) {
        fprintf(stderr, "No backend or frontend found in: %s\n", exe_dir);
        release_single_instance();
        return 1;
    }
    
    char url[1024] = {0};
    int needs_health_check = 0;
    
    if (disco.has_frontend) {
#ifdef _WIN32
        snprintf(url, sizeof(url), "file:///%s", disco.frontend_file);
        for (char *p = url; *p; p++) if (*p == '\\') *p = '/';
#else
        snprintf(url, sizeof(url), "file://%s", disco.frontend_file);
#endif
    } else if (disco.has_backend) {
        snprintf(url, sizeof(url), "http://127.0.0.1:%d", disco.port);
        needs_health_check = 1;
    }
    
    if (disco.has_backend) {
        if (spawn_backend(disco.backend_cmd, disco.backend_dir, disco.port) != 0) {
            fprintf(stderr, "Failed to spawn backend: %s\n", disco.backend_cmd);
        }
    }
    
    if (needs_health_check) {
        char health_url[512];
        snprintf(health_url, sizeof(health_url), "http://127.0.0.1:%d/health", disco.port);
        if (wait_for_health(health_url, 15000) != 0) {
            snprintf(health_url, sizeof(health_url), "http://127.0.0.1:%d/", disco.port);
            if (wait_for_health(health_url, 5000) != 0) {
                fprintf(stderr, "Backend health check failed.\n");
            }
        }
    }
    
    webview_t w = webview_create(0, NULL);
    if (!w) {
        fprintf(stderr, "Failed to create webview.\n");
        kill_backend();
        release_single_instance();
        return 1;
    }
    
    webview_set_title(w, "UniTML");
    webview_set_size(w, 1024, 768, WEBVIEW_HINT_NONE);
    webview_bind(w, "nativePing", native_ping, w);
    webview_navigate(w, url);
    webview_run(w);
    
    webview_destroy(w);
    kill_backend();
    release_single_instance();
    
    return 0;
}
