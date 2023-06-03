#include <stdio.h>
#include <sys/prctl.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <mqueue.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <assert.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <sys/stat.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

#include <camera_HAL.hpp>

#include <toy_message.h>
#include <shared_memory.h>

#define BUF_LEN 1024
#define TOY_TEST_FS "./fs"

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

#define SENSOR_DATA 2

void *monitor_thread(void* arg)
{
    char *s = arg;
    int mqretcode;
    toy_msg_t msg;
    int shmid;

    printf("%s", s);

    mqretcode = (int)mq_receive(monitor_queue, (void *)&msg, sizeof(toy_msg_t), 0);
    assert(mqretcode >= 0);
    shmid = msg.param1;

    // void *addr = shmat(shmid, NULL, 0);

    shm_sensor_t data;

    while (1) {
        mqretcode = (int)mq_receive(monitor_queue, (void *)&msg, sizeof(toy_msg_t), 0);
        assert(mqretcode >= 0);

        printf("monitor_thread: 메시지가 도착했습니다.\n");
        printf("msg.type: %d\n", msg.msg_type);
        printf("msg.param1: %d\n", msg.param1);
        printf("msg.param2: %d\n", msg.param2);

        if (msg.msg_type == SENSOR_DATA) {
            // memcpy(&data, addr, sizeof(shm_sensor_t));

            printf("temp: %d\n", data.temp);
            printf("press: %d\n", data.press);
            printf("humidity: %d\n", data.humidity);
        }
    }

    return 0;
}

// https://stackoverflow.com/questions/21618260/how-to-get-total-size-of-subdirectories-in-c
static long get_directory_size(char *dirname)
{
    DIR *dir = opendir(dirname);
    if (dir == 0)
        return 0;

    struct dirent *dit;
    struct stat st;
    long size = 0;
    long total_size = 0;
    char filePath[1024];

    while ((dit = readdir(dir)) != NULL) {
        if ( (strcmp(dit->d_name, ".") == 0) || (strcmp(dit->d_name, "..") == 0) )
            continue;

        sprintf(filePath, "%s/%s", dirname, dit->d_name);
        if (lstat(filePath, &st) != 0)
            continue;
        size = st.st_size;

        if (S_ISDIR(st.st_mode)) {
            long dir_size = get_directory_size(filePath) + size;
            total_size += dir_size;
        } else {
            total_size += size;
        }
    }
    return total_size;
}

void *disk_service_thread(void* arg)
{
    char *s = arg;
    int inotifyFd, wd, j;
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    ssize_t numRead;
    char *p;
    struct inotify_event *event;
    int total_size;

    printf("%s", s);

    inotifyFd = inotify_init();

    wd = inotify_add_watch(inotifyFd, TOY_TEST_FS, IN_CREATE);

    while(1)
    {
        numRead = read(inotifyFd, buf, BUF_LEN);

        if(numRead == 0)
        {
            printf("read() from inotify fd returned 0!\n");
        }

        for (p = buf; p < buf + numRead;)
        {
            event = (struct inotify_event *) p;
            p += sizeof(struct inotify_event) + event->len;
        }
        total_size = get_directory_size(TOY_TEST_FS);
        printf("directory size: %d\n", total_size);
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
