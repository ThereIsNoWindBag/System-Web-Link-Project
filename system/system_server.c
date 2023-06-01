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

#include <camera_HAL.hpp>

pthread_mutex_t system_loop_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  system_loop_cond  = PTHREAD_COND_INITIALIZER;
bool            system_loop_exit = false;    ///< true if main loop should exit

static int toy_timer = 0;

void signal_exit(void);

static void timer_expire_signal_handler()
{
    toy_timer++;
    signal_exit();
}

void set_periodic_timer(long sec_delay, long usec_delay)
{
	struct itimerval itimer_val = {
		 .it_interval = { .tv_sec = sec_delay, .tv_usec = usec_delay },
		 .it_value = { .tv_sec = sec_delay, .tv_usec = usec_delay }
    };

	setitimer(ITIMER_REAL, &itimer_val, (struct itimerval*)0);
}

int posix_sleep_ms(unsigned int timeout_ms)
{
    struct timespec sleep_time;

    sleep_time.tv_sec = timeout_ms / MILLISEC_PER_SECOND;
    sleep_time.tv_nsec = (timeout_ms % MILLISEC_PER_SECOND) * (NANOSEC_PER_USEC * USEC_PER_MILLISEC);

    return nanosleep(&sleep_time, NULL);
}

void *watchdog_thread(void* arg)
{
    char *s = arg;

    printf("%s", s);

    while (1) {
        posix_sleep_ms(5000);
    }

    return 0;
}

void *monitor_thread(void* arg)
{
    char *s = arg;

    printf("%s", s);

    while (1) {
        posix_sleep_ms(5000);
    }

    return 0;
}

void *disk_service_thread(void* arg)
{
    char *s = arg;

    printf("%s", s);

    while (1) {
        posix_sleep_ms(5000);
    }

    return 0;
}

void *camera_service_thread(void* arg)
{
    char *s = arg;

    printf("%s", s);

   toy_camera_open();
   toy_camera_take_picture();

    while (1) {
        posix_sleep_ms(5000);
    }

    return 0;
}

void signal_exit(void)
{
    /* 여기에 구현하세요..  종료 메시지를 보내도록.. */
    pthread_mutex_lock(&system_loop_mutex);
    system_loop_exit = true;
    pthread_cond_signal(&system_loop_cond);
    pthread_mutex_unlock(&system_loop_mutex);
}

int system_server()
{
    struct itimerval itv;
    struct sigaction sa;

    printf("나 system_server 프로세스!\n");

    /* 5초 타이머를 만들어 봅시다. */
    signal(SIGALRM, timer_expire_signal_handler);
    /* 10초 타이머 등록 */
    set_periodic_timer(10, 0);

    pthread_t watchdog_thread_tid, monitor_thread_tid, disk_service_thread_tid, camera_service_thread_tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&watchdog_thread_tid, &attr, watchdog_thread, "watchdog thread initiated");
    pthread_create(&monitor_thread_tid, &attr, monitor_thread, "monitor thread initiated");
    pthread_create(&disk_service_thread_tid, &attr, disk_service_thread, "disk thread initiated");
    pthread_create(&camera_service_thread_tid, &attr, camera_service_thread, "camera thread initiated");

    // 여기에 구현하세요... 여기서 cond wait로 대기한다. 10초 후 알람이 울리면 <== system 출력
    // /* 1초 마다 wake-up 한다 */
    pthread_mutex_lock(&system_loop_mutex);
    while (system_loop_exit == false) {
        pthread_cond_wait(&system_loop_cond, &system_loop_mutex);
    }
    printf("10 seconds elapsed!\n");
    pthread_mutex_unlock(&system_loop_mutex);

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
        prctl(PR_SET_NAME, name, NULL, NULL, NULL);
        system_server();
    }

    return system_pid;
}
