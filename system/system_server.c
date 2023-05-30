#include <stdio.h>
#include <sys/prctl.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

int posix_sleep_ms(unsigned int timeout_ms)
{
    struct timespec sleep_time;

    sleep_time.tv_sec = timeout_ms / MILLISEC_PER_SECOND;
    sleep_time.tv_nsec = (timeout_ms % MILLISEC_PER_SECOND) * (NANOSEC_PER_USEC * USEC_PER_MILLISEC);

    return nanosleep(&sleep_time, NULL);
}

void five_sec_alarm_handler(int sig)
{
    printf("%ld seconds\n", clock() / CLOCKS_PER_SEC);
} 

static void *simple_thread(void* arg)
{
    char *s = arg;
    
    printf("%s\n", s);

    while(1)
    {
        sleep(1);
    }
}


int system_server()
{
    struct itimerval itv;
    struct sigaction sa;

    printf("나 system_server 프로세스!\n");

    /* 5초 타이머를 만들어 봅시다. */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = five_sec_alarm_handler;
    sigaction(SIGALRM, &sa, NULL);
    // signal(SIGALRM, five_sec_alarm_handler);

    itv.it_value.tv_sec = 1; // 이걸 0으로 하면 문제가 발생한다. 왜일까?
    itv.it_value.tv_usec = 0;
    itv.it_interval.tv_sec = 5;
    itv.it_interval.tv_usec = 0;

    pthread_t watchdog_thread_tid, monitor_thread_tid, disk_service_thread_tid, camera_service_thread_tid;
    pthread_attr_t attr;

    setitimer(ITIMER_REAL, &itv, NULL);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&watchdog_thread_tid, &attr, simple_thread, "watchdog thread initiated");
    pthread_create(&monitor_thread_tid, &attr, simple_thread, "monitor thread initiated");
    pthread_create(&disk_service_thread_tid, &attr, simple_thread, "disk thread initiated");
    pthread_create(&camera_service_thread_tid, &attr, simple_thread, "camera thread initiated");

    while (1) {
        posix_sleep_ms(5000);
    }

    return 0;
}

pid_t create_system_server()
{
    pid_t system_pid;
    const char *name = "system_server";

    printf("여기서 시스템 프로세스를 생성합니다.\n");
    system_pid = fork();

    if(system_pid == 0)
    {
        system_server();
    }

    return system_pid;
}
