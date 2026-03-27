#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/dev/socket/abootpatch_daemon"
#define REQ_ROOT 2

static int req_root(const char *pkg) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return 0;
    struct sockaddr_un a; memset(&a,0,sizeof(a));
    a.sun_family = AF_UNIX;
    strncpy(a.sun_path, SOCKET_PATH, sizeof(a.sun_path)-1);
    if (connect(fd,(struct sockaddr*)&a,sizeof(a))<0) { close(fd); return 0; }
    int req=REQ_ROOT; write(fd,&req,sizeof(req));
    int l=strlen(pkg); write(fd,&l,sizeof(l)); write(fd,pkg,l);
    int r; read(fd,&r,sizeof(r)); close(fd);
    return r==0;
}

int main(int argc, char *argv[]) {
    uid_t uid = getuid();
    if (uid == 0) {
        if (argc > 1) execvp(argv[1], argv+1);
        else execl("/system/bin/sh","sh",NULL);
        return 1;
    }
    char pkg[64]; snprintf(pkg, sizeof(pkg), "uid_%d", uid);
    if (!req_root(pkg)) { fprintf(stderr, "su: denied\n"); return 1; }
    if (setgid(0)!=0||setuid(0)!=0) { fprintf(stderr,"su: setuid failed\n"); return 1; }
    if (argc > 1) execvp(argv[1], argv+1);
    else execl("/system/bin/sh","sh",NULL);
    return 1;
}
