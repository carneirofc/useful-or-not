#include <errno.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/kernel.h> /* for struct sysinfo */
#include <linux/unistd.h> /* for _syscallX macros/related stuff */
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/**************************************************************************
 *      /proc/[pid]/stat https://man7.org/linux/man-pages/man5/proc.5.html
 *
 *             (1) pid  %d
 *             (2) comm  %s
 *             (3) state  %c
 *             (4) ppid  %d
 *             (5) pgrp  %d
 *             (6) session  %d
 *             (7) tty_nr  %d
 *             (8) tpgid  %d
 *             (9) flags  %u
 *             (10) minflt  %lu
 *             (11) cminflt  %lu
 *             (12) majflt  %lu
 *             (13) cmajflt  %lu
 *             (14) utime  %lu
 *             (15) stime  %lu
 *             (16) cutime  %ld
 *             (17) cstime  %ld
 *             (18) priority  %ld
 *             (19) nice  %ld
 *             (20) num_threads  %ld
 *             (21) itrealvalue  %ld
 *             (22) starttime  %llu
 *             (23) vsize  %lu
 *             (24) rss  %ld
 *             (25) rsslim  %lu
 *             (26) startcode  %lu  [PT]
 *             (27) endcode  %lu  [PT]
 *             (28) startstack  %lu  [PT]
 *             (29) kstkesp  %lu  [PT]
 *             (30) kstkeip  %lu  [PT]
 *             (31) signal  %lu
 *             (32) blocked  %lu
 *             (33) sigignore  %lu
 *             (34) sigcatch  %lu
 *             (35) wchan  %lu  [PT]
 *             (36) nswap  %lu
 *             (37) cnswap  %lu
 *             (38) exit_signal  %d  (since Linux 2.1.22)
 *             (39) processor  %d  (since Linux 2.2.8)
 *             (40) rt_priority  %u  (since Linux 2.5.19)
 *             (41) policy  %u  (since Linux 2.5.19)
 *             (42) delayacct_blkio_ticks  %llu  (since Linux 2.6.18)
 *             (43) guest_time  %lu  (since Linux 2.6.24)
 *             (44) cguest_time  %ld  (since Linux 2.6.24)
 *             (45) start_data  %lu  (since Linux 3.3)  [PT]
 *             (46) end_data  %lu  (since Linux 3.3)  [PT]
 *             (47) start_brk  %lu  (since Linux 3.3)  [PT]
 *             (48) arg_start  %lu  (since Linux 3.5)  [PT]
 *             (49) arg_end  %lu  (since Linux 3.5)  [PT]
 *             (50) env_start  %lu  (since Linux 3.5)  [PT]
 *             (51) env_end  %lu  (since Linux 3.5)  [PT]
 *             (52) exit_code  %d  (since Linux 3.5)  [PT]
 *************************************************************************/
typedef struct {
    char               name[1000];
    long               cutime, cstime;
    int                pid;
    struct sysinfo     s_info;
    unsigned long long starttime;
    unsigned long      utime, stime;
} sys_stat_t;

void update_proc_stat(int pid, sys_stat_t *sys_stat) {
    // Get process status
    char filename[1000];
    sprintf(filename, "/proc/%d/stat", pid);
    FILE *fd = fopen(filename, "r");
    if (fd == NULL) {
        printf("Failed to get %d stat\n", pid);
        exit(1);
    }
    // clang-format off
    fscanf(fd,
           /*(1) pid          */ "%d"
           /*(2) comm         */ " %s"
           /*(3) state        */ " %*c"
           /*(4) ppid         */ " %*d"
           /*(5) pgrp         */ " %*d"
           /*(6) session      */ " %*d"
           /*(7) tty_nr       */ " %*d"
           /*(8) tpgid        */ " %*d"
           /*(9) flags        */ " %*u"
           /*(10) minflt      */ " %*lu"
           /*(11) cminflt     */ " %*lu"
           /*(12) majflt      */ " %*lu"
           /*(13) cmajflt     */ " %*lu"
           /*(14) utime       */ " %lu"
           /*(15) stime       */ " %lu"
           /*(16) cutime      */ " %ld"
           /*(17) cstime      */ " %ld"
           /*(18) priority    */ " %*ld"
           /*(19) nice        */ " %*ld"
           /*(20) num_threads */ " %*ld"
           /*(21) itrealvalue */ " %*ld"
           /*(22) starttime   */ " %llu",
           &sys_stat->pid, sys_stat->name, &sys_stat->utime, &sys_stat->stime,
           &sys_stat->cutime, &sys_stat->cstime, &sys_stat->starttime);
    // clang-format on
    fclose(fd);
}

void get_uptime(double *uptime);

/**
 * Calculate a process total cpu usage.
 * */
void get_cpu_usage(sys_stat_t *sys_stat, double uptime, double freq,
                   double *process_cpu_time, double *process_lifetime,
                   double *cpu_usage) {
    // First we determine the total time spent for the process:
    *process_cpu_time = (sys_stat->utime + sys_stat->stime + sys_stat->cutime +
                         sys_stat->cstime) /
                        freq;

    // Next we get the total elapsed time in seconds since the process started:
    *process_lifetime = uptime - (sys_stat->starttime / freq);

    // Finally we calculate the CPU usage percentage:
    *cpu_usage = 100. * (*process_cpu_time / *process_lifetime);
}

/*
void get_cpu_usage_delta(int pid, sys_stat_t *sys_stat, double freq, int msec) {
    struct timespec ts;
    int             res;
    if (msec < 0) {
        fprintf(stderr, "Invalid sleep range\n");
        exit(1);
    }

    ts.tv_sec         = msec / 1000;
    ts.tv_nsec        = (msec % 1000) * 1000000;

    double uptime_ini;
    get_uptime(&uptime_ini);
    update_proc_stat(pid, sys_stat);
    double process_cpu_time_ini = (sys_stat->utime + sys_stat->stime +
                                   sys_stat->cutime + sys_stat->cstime) /
                                  freq;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    double uptime_end;
    update_proc_stat(pid, sys_stat);
    get_uptime(&uptime_end);
    double process_cpu_time_end = (sys_stat->utime + sys_stat->stime +
                                   sys_stat->cutime + sys_stat->cstime) /
                                  freq;

    double cpu_usage_delta =
        100. * ((process_cpu_time_end - process_cpu_time_ini) /
                (uptime_end - uptime_ini));

    printf("Cpu delta for %dms (%lf %lf)=%lf\n", msec, process_cpu_time_ini,
           process_cpu_time_end,cpu_usage_delta
    );
}*/

void get_uptime(double *uptime) {
    FILE *fd = fopen("/proc/uptime", "r");
    if (fd == NULL) {
        printf("Failed to read uptime\n");
        exit(1);
    }
    if (errno) {
        fprintf(stderr, "Failed to read uptime\n");
        exit(1);
    }
    int res = fscanf(fd, "%lf %*f", uptime);
    if (res != 1) {
        fprintf(stderr, "Failed to read uptime\n");
        exit(1);
    }
    fclose(fd);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage:\n"
                        "\t1) PID\n");
        exit(1);
    }
    int pid;
    sscanf(argv[1], "%d", &pid);
    printf("PID %d\n", pid);

    long       freq;
    double     uptime;
    double     process_cpu_time, process_lifetime, cpu_usage;
    sys_stat_t sys_stat;
    freq = sysconf(_SC_CLK_TCK);

    printf("PID,Name,CPU_Time,Lifetime,Total_CPU(%%)\n");
    for (;;) {
        // get_cpu_usage_delta(pid, &sys_stat, freq, 5000);
        get_uptime(&uptime);
        update_proc_stat(pid, &sys_stat);
        get_cpu_usage(&sys_stat, uptime, freq, &process_cpu_time,
                      &process_lifetime, &cpu_usage);
        printf("%d,%s,%lf,%lf,%lf\n", sys_stat.pid, sys_stat.name,
               process_cpu_time, process_lifetime, cpu_usage);
    }
    return 0;
}
