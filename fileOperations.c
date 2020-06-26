#include <errno.h>
#include <fcntl.h>
#include <linux/kernel.h> /* for struct sysinfo */
#include <linux/unistd.h> /* for _syscallX macros/related stuff */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
    double uptime;
    int    ret;
    FILE * f = fopen("/proc/uptime", "r");
    if (f == NULL) {
        printf("Failed to read uptime\n");
        exit(1);
    }
    ret = fscanf(f, "%lf %*f", &uptime);
    printf("Uptime %f  %i\n", uptime, ret);

    uptime = 0;

    sleep(1);
    ret = fscanf(f, "%lf %*f", &uptime);
    printf("no rewind: Uptime %f  %i\n", uptime, ret);

    sleep(1);
    rewind(f);

    ret = fscanf(f, "%lf %*f", &uptime);
    printf("rewind: Uptime %f  %i\n", uptime, ret);

    fclose(f);
    return 0;
}
