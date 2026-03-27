#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>

#define ABDIR   "/data/adb/abootpatch"
#define MODDIR  "/data/adb/modules"
#define DAEMON  "/data/adb/abootpatch/abootpatchd"

static void safe_mkdir(const char *p) { mkdir(p, 0755); }
static int fexists(const char *p) { return access(p,F_OK)==0; }

static void mount_modules(void) {
    DIR *d = opendir(MODDIR); if (!d) return;
    struct dirent *e;
    while ((e=readdir(d))!=NULL) {
        if (e->d_name[0]=='.') continue;
        char dis[512], ov[512];
        snprintf(dis, sizeof(dis), "%s/%s/disable", MODDIR, e->d_name);
        snprintf(ov,  sizeof(ov),  "%s/%s/system",  MODDIR, e->d_name);
        if (!fexists(dis) && fexists(ov))
            mount(ov, "/system", NULL, MS_BIND|MS_RDONLY, NULL);
    }
    closedir(d);
}

static void run_scripts(const char *script_name) {
    DIR *d = opendir(MODDIR); if (!d) return;
    struct dirent *e;
    while ((e=readdir(d))!=NULL) {
        if (e->d_name[0]=='.') continue;
        char sc[512], dis[512];
        snprintf(sc,  sizeof(sc),  "%s/%s/%s", MODDIR, e->d_name, script_name);
        snprintf(dis, sizeof(dis), "%s/%s/disable", MODDIR, e->d_name);
        if (!fexists(dis) && fexists(sc)) {
            if (fork()==0) { execl("/system/bin/sh","sh",sc,NULL); exit(1); }
        }
    }
    closedir(d);
    int st; while (waitpid(-1,&st,WNOHANG)>0);
}

static void start_daemon(void) {
    if (!fexists(DAEMON)) return;
    if (fork()==0) {
        setsid();
        int n = open("/dev/null",O_RDWR);
        dup2(n,0); dup2(n,1); dup2(n,2); close(n);
        execl(DAEMON,"abootpatchd",NULL); exit(1);
    }
}

int main(int argc, char *argv[]) {
    safe_mkdir("/dev/abootpatch");
    mount("tmpfs","/dev/abootpatch","tmpfs",0,"mode=755");
    mount_modules();
    run_scripts("post-fs-data.sh");
    start_daemon();
    if (fexists("/init_real")) { argv[0]=(char*)"/init_real"; execv("/init_real",argv); }
    argv[0]=(char*)"/system/bin/init"; execv("/system/bin/init",argv);
    return 1;
}
