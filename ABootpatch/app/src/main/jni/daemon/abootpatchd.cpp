#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>

#define SOCKET_PATH "/dev/socket/abootpatch_daemon"
#define POLICY_DIR  "/data/adb/abootpatch/policy"
#define REQ_STATUS  1
#define REQ_ROOT    2
#define REQ_REBOOT  4
#define RESP_OK     0
#define RESP_DENY   1
#define RESP_ERR   -1

static int g_sfd = -1;

static void on_exit_sig(int s) { if(g_sfd>=0) close(g_sfd); unlink(SOCKET_PATH); exit(0); }
static void write_int(int fd, int v) { write(fd, &v, sizeof(v)); }
static int  read_int(int fd)  { int v=RESP_ERR; read(fd, &v, sizeof(v)); return v; }
static void read_str(int fd, char *buf, int max) {
    int l = read_int(fd);
    if (l<=0||l>=max) { buf[0]=0; return; }
    read(fd, buf, l); buf[l]=0;
}

static int check_policy(const char *pkg) {
    char p[256]; snprintf(p, sizeof(p), "%s/%s", POLICY_DIR, pkg);
    FILE *f = fopen(p, "r");
    if (!f) return 0;
    char line[128]; int ok=0;
    while (fgets(line, sizeof(line), f))
        if (strncmp(line, "policy=allow", 12)==0) { ok=1; break; }
    fclose(f); return ok;
}

static void *handle_client(void *arg) {
    int fd = *(int*)arg; free(arg);
    int req = read_int(fd);
    switch (req) {
        case REQ_STATUS: write_int(fd, RESP_OK); break;
        case REQ_ROOT: {
            char pkg[256]; read_str(fd, pkg, sizeof(pkg));
            write_int(fd, check_policy(pkg) ? RESP_OK : RESP_DENY);
            break;
        }
        case REQ_REBOOT: {
            char mode[32]; read_str(fd, mode, sizeof(mode));
            write_int(fd, RESP_OK); close(fd);
            if (strcmp(mode,"recovery")==0)        system("/system/bin/reboot recovery");
            else if (strcmp(mode,"bootloader")==0) system("/system/bin/reboot bootloader");
            else                                    system("/system/bin/reboot");
            return NULL;
        }
        default: write_int(fd, RESP_ERR);
    }
    close(fd); return NULL;
}

int main(void) {
    signal(SIGTERM, on_exit_sig); signal(SIGINT, on_exit_sig); signal(SIGPIPE, SIG_IGN);
    mkdir(POLICY_DIR, 0700); unlink(SOCKET_PATH);
    g_sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_sfd < 0) return 1;
    struct sockaddr_un addr; memset(&addr,0,sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    if (bind(g_sfd,(struct sockaddr*)&addr,sizeof(addr))<0) return 1;
    chmod(SOCKET_PATH, 0666); listen(g_sfd, 16);
    while (1) {
        int cfd = accept(g_sfd, NULL, NULL);
        if (cfd < 0) continue;
        int *p = (int*)malloc(sizeof(int)); *p = cfd;
        pthread_t tid; pthread_create(&tid, NULL, handle_client, p); pthread_detach(tid);
    }
    return 0;
}
