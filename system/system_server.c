#include <stdio.h>
#include <sys/prctl.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <mqueue.h>
#include <semaphore.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

#include <camera_HAL.hpp>

#include <toy_message.h>

pthread_mutex_t system_loop_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  system_loop_cond  = PTHREAD_COND_INITIALIZER;
bool            system_loop_exit = false;    ///< true if main loop should exit

static mqd_t watchdog_queue;
static mqd_t monitor_queue;
static mqd_t disk_queue;
static mqd_t camera_queue;

static int toy_timer = 0;
static sem_t global_timer_sem;
static bool global_timer_stopped;

static void timer_expire_signal_handler()
{
    sem_post(&global_timer_sem);
}

static void system_timeout_handler()
{
    toy_timer++;
    printf("toy_timer: %d\n", toy_timer);
}

void set_periodic_timer(long sec_delay, long usec_delay)
{
	struct itimerval itimer_val = {
		 .it_interval = { .tv_sec = sec_delay, .tv_usec = usec_delay },
		 .it_value = { .tv_sec = sec_delay, .tv_usec = usec_delay }
    };

	setitimer(ITIMER_REAL, &itimer_val, (struct itimerval*)0);
}

static void *timer_thread(void *not_used)
{
    signal(SIGALRM, timer_expire_signal_handler);
    set_periodic_timer(1, 0);

	while (!global_timer_stopped) {
        sem_wait(&global_timer_sem);
		system_timeout_handler();
	}

	return 0;
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
    int mqretcode;
    toy_msg_t msg;

    printf("%s", s);

    while (1) {
        mq_receive(watchdog_queue, (char *) &msg, sizeof msg, NULL);
    }

    return 0;
}

void *monitor_thread(void* arg)
{
    char *s = arg;
    int mqretcode;
    toy_msg_t msg;

    printf("%s", s);

    while (1) {
        mq_receive(monitor_queue, (char *) &msg, sizeof msg, NULL);
    }

    return 0;
}

void *disk_service_thread(void* arg)
{
    char *s = arg;
    FILE* apipe;
    char buf[1024];
    char cmd[]="df -h ./" ;

    int mqretcode;
    toy_msg_t msg;

    printf("%s", s);

    while (1) 
    {
        mq_receive(disk_queue, (char *) &msg, sizeof msg, NULL);
    }

    return 0;
}

#define CAMERA_TAKE_PICTURE 1

void *camera_service_thread(void* arg)
{
    char *s = arg;
    int mqretcode;
    toy_msg_t msg;

    printf("%s", s);

    toy_camera_open();

    ssize_t t;

    while (1) {
        t = mq_receive(camera_queue, (char *) &msg, sizeof(toy_msg_t), 0);
        printf("param1 : %d\n", msg.param1);
        printf("param2 : %d\n", msg.param2);
        if(msg.msg_type == CAMERA_TAKE_PICTURE)
            toy_camera_take_picture();
    }

    return 0;
}

int system_server()
{
    printf("나 system_server 프로세스!\n");

    sem_init(&global_timer_sem, 0, 0);

    // 각 쓰레드를 위한 메세지 큐 생성
    watchdog_queue = mq_open("/watchdog_queue", O_RDWR);
    monitor_queue = mq_open("/monitor_queue", O_RDWR);
    disk_queue = mq_open("/disk_queue", O_RDWR);
    camera_queue = mq_open("/camera_queue", O_RDWR);

    // 쓰레드용 변수
    pthread_t watchdog_thread_tid, monitor_thread_tid, disk_service_thread_tid, camera_service_thread_tid, timer_thread_tid;
    pthread_attr_t attr;

    // 쓰레드 설정
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // 쓰레드 생성
    pthread_create(&watchdog_thread_tid, &attr, watchdog_thread, "watchdog thread initiated");
    pthread_create(&monitor_thread_tid, &attr, monitor_thread, "monitor thread initiated");
    pthread_create(&disk_service_thread_tid, &attr, disk_service_thread, "disk thread initiated");
    pthread_create(&camera_service_thread_tid, &attr, camera_service_thread, "camera thread initiated");
    pthread_create(&timer_thread_tid, &attr, timer_thread, "timer thread\n");

    while (1) {
        posix_sleep_ms(10000);
    }

    return 0;
}

pid_t create_system_server()
{
    printf("여기서 시스템 프로세스를 생성합니다.\n");

    // 변수 선언
    pid_t system_pid;
    const char *name = "system_server";

    // fork() 분기
    system_pid = fork();

    // 자식이면 프로세스 이름 바꾸고
    // system_server()로 진입
    if(system_pid == 0)
    {
        prctl(PR_SET_NAME, name, NULL, NULL, NULL);
        system_server();
    }

    // 부모면 안녕~
    return system_pid;
}
