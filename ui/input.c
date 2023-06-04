#include <stdio.h>
#include <sys/prctl.h>
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>
#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <mqueue.h>
#include <assert.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <linux/seccomp.h>
#include <seccomp.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>
#include <toy_message.h>
#include <shared_memory.h>

#define TOY_TOK_BUFSIZE 64
#define TOY_TOK_DELIM " \t\r\n\a"
#define TOY_BUFFSIZE 1024
#define DUMP_STATE 2

static int shmid;

typedef struct _sig_ucontext {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
} sig_ucontext_t;

static pthread_mutex_t global_message_mutex  = PTHREAD_MUTEX_INITIALIZER;
static char global_message[TOY_BUFFSIZE];

static mqd_t watchdog_queue;
static mqd_t monitor_queue;
static mqd_t disk_queue;
static mqd_t camera_queue;

void segfault_handler(int sig_num, siginfo_t * info, void * ucontext) {
  void * array[50];
  void * caller_address;
  char ** messages;
  int size, i;
  sig_ucontext_t * uc;

  uc = (sig_ucontext_t *) ucontext;

  /* Get the address at the time the signal was raised */
  caller_address = (void *) uc->uc_mcontext.rip;  // RIP: x86_64 specific     arm_pc: ARM

  fprintf(stderr, "\n");

  if (sig_num == SIGSEGV)
    printf("signal %d (%s), address is %p from %p\n", sig_num, strsignal(sig_num), info->si_addr,
           (void *) caller_address);
  else
    printf("signal %d (%s)\n", sig_num, strsignal(sig_num));

  size = backtrace(array, 50);
  /* overwrite sigaction with caller's address */
  array[1] = caller_address;
  messages = backtrace_symbols(array, size);

  /* skip first stack frame (points here) */
  for (i = 1; i < size && messages != NULL; ++i) {
    printf("[bt]: (%d) %s\n", i, messages[i]);
  }

  free(messages);

  exit(EXIT_FAILURE);
}

/*
 *  sensor thread
 */
void *sensor_thread(void* arg)
{
    int mqretcode;
    char *s = arg;
    toy_msg_t msg;

    printf("%s", s);

    // void *addr = shmat(shmid, NULL, 0);

    // send shm_key
    msg.msg_type = 1;
    msg.param1 = shmid;
    msg.param2 = 0;
    mqretcode = mq_send(monitor_queue, (char *)&msg, sizeof(msg), 0);
    assert(mqretcode == 0);

    shm_sensor_t data;
    data.temp = 0;
    data.press = 0;
    data.humidity = 0;

    while (1) {
        // memcpy(addr, &data, sizeof(shm_sensor_t));
        
        msg.msg_type = 2;
        msg.param1 = 0;
        msg.param2 = 0;
        mqretcode = mq_send(monitor_queue, (char *)&msg, sizeof(msg), 0);
        assert(mqretcode == 0);

        data.temp++;
        data.press += 2;
        data.humidity += 4;

        posix_sleep_ms(5000);
    }

    return 0;
}

/*
 *  command thread
 */

int toy_send(char **args);
int toy_mutex(char **args);
int toy_shell(char **args);
int toy_message_queue(char **args);
int toy_read_elf_header(char **args);
int toy_dump_state(char **args);
int toy_mincore(char **args);
int toy_exit(char **args);

char *builtin_str[] = {
    "send",
    "mu",
    "sh",
    "mq",
    "elf",
    "dump",
    "mincore",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &toy_send,
    &toy_mutex,
    &toy_shell,
    &toy_message_queue,
    &toy_read_elf_header,
    &toy_dump_state,
    &toy_mincore,
    &toy_exit
};

int toy_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int toy_send(char **args)
{
    printf("send message: %s\n", args[1]);

    return 1;
}

int toy_mutex(char **args)
{
    if (args[1] == NULL) {
        return 1;
    }

    printf("save message: %s\n", args[1]);
    pthread_mutex_lock(&global_message_mutex);
    strcpy(global_message, args[1]);
    pthread_mutex_unlock(&global_message_mutex);
    return 1;
}

int toy_message_queue(char **args)
{
    int mqretcode;
    toy_msg_t msg;

    if (args[1] == NULL || args[2] == NULL) {
        return 1;
    }

    if (!strcmp(args[1], "camera")) {
        msg.msg_type = atoi(args[2]);
        msg.param1 = 0;
        msg.param2 = 0;
        mqretcode = mq_send(camera_queue, (char *)&msg, sizeof(msg), 0);
        assert(mqretcode == 0);
    }

    return 1;
}

int toy_read_elf_header(char **args)
{
    int mqretcode;
    toy_msg_t msg;
    int in_fd;
    char *contents = NULL;
    size_t contents_sz;
    struct stat st;
    Elf64Hdr elf_header;

    in_fd = open("./sample/sample.elf", O_RDONLY);
	if ( in_fd < 0 ) {
        printf("cannot open ./sample/sample.elf\n");
        return 1;
    }
    void *addr = mmap(NULL, sizeof(Elf64Hdr), PROT_READ, MAP_PRIVATE, in_fd, 0);
    memcpy(&elf_header, addr, sizeof(Elf64Hdr));

        printf("ELF file information\n");
    printf("    Type: %d\n", elf_header.e_type);
    printf("    Machine: %d\n", elf_header.e_machine);
    printf("    Entry point address: %d\n", elf_header.e_entry);

    printf("    Program header offset: %d\n", elf_header.e_phoff);
    printf("    Number of program headers: %d\n", elf_header.e_phnum);
    printf("    Size of program headers: %d\n", elf_header.e_phentsize);

    printf("    Section of section headers: %d\n", elf_header.e_shoff);
    printf("    Number of section headers: %d\n", elf_header.e_shnum);
    printf("    Size of section headers: %d\n", elf_header.e_shentsize);

    return 1;
}

int toy_dump_state(char **args)
{
    int mqretcode;
    toy_msg_t msg;

    msg.msg_type = DUMP_STATE;
    msg.param1 = 0;
    msg.param2 = 0;
    mqretcode = mq_send(camera_queue, (char *)&msg, sizeof(msg), 0);
    assert(mqretcode == 0);
    mqretcode = mq_send(monitor_queue, (char *)&msg, sizeof(msg), 0);
    assert(mqretcode == 0);

    return 1;
}

int toy_mincore(char **args)
{
    unsigned char vec[20];
    int res;
    size_t page = sysconf(_SC_PAGESIZE);
    void *addr = mmap(NULL, 20 * page, PROT_READ | PROT_WRITE,
                    MAP_NORESERVE | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    res = mincore(addr, 10 * page, vec);
    assert(res == 0);

    return 1;
}


int toy_exit(char **args)
{
    return 0;
}

int toy_shell(char **args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("toy");
        }
        exit(EXIT_FAILURE);
    } 
    else if (pid < 0) {
        perror("toy");
    } 
    else
    {
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int toy_execute(char **args)
{
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < toy_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return 1;
}

char *toy_read_line(void)
{
    char *line = NULL;
    ssize_t bufsize = 0;

    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        } else {
            perror(": getline\n");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

char **toy_split_line(char *line)
{
    int bufsize = TOY_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "toy: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOY_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += TOY_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                free(tokens_backup);
                fprintf(stderr, "toy: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOY_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void toy_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("TOY>");
        line = toy_read_line();
        args = toy_split_line(line);
        status = toy_execute(args);

        free(line);
        free(args);
    } while (status);
}

void *command_thread(void* arg)
{
    char *s = arg;

    printf("%s\n", s);

    toy_loop();

    return 0;
}

int input()
{
    printf("나 input 프로세스!\n");

    // seccomp
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);

    /* Cause clone() and fork() to fail, each with different errors */

    int rc = seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(mincore), 0);

    /* Install the seccomp filter into the kernel */

    rc = seccomp_load(ctx);

    /* Free the user-space seccomp filter state */

    seccomp_release(ctx);

    // signal
    struct sigaction sa;
    memset(&sa, 0, sizeof(sigaction));
    sigemptyset(&sa.sa_mask);

    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sa.sa_sigaction = segfault_handler;

    // input 프로세스에 seg_fault 시그널이 왔을 때 위의 코드를 실행
    sigaction(SIGSEGV, &sa, NULL); /* ignore whether it works or not */

    shmid = shmget(IPC_PRIVATE, sizeof(shm_sensor_t), IPC_CREAT | 0666);

    watchdog_queue = mq_open("/watchdog_queue", O_RDWR);
    assert(watchdog_queue);
    monitor_queue = mq_open("/monitor_queue", O_RDWR);
    assert(monitor_queue);
    disk_queue = mq_open("/disk_queue", O_RDWR);
    assert(disk_queue);
    camera_queue = mq_open("/camera_queue", O_RDWR);
    assert(camera_queue);

    pthread_t command_thread_tid, sensor_thread_tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&command_thread_tid, &attr, command_thread, "command thread initiated");
    pthread_create(&sensor_thread_tid, &attr, sensor_thread, "sensor thread initiated");

    while (1) {
        sleep(1);
    }

    return 0;
}

pid_t create_input()
{
    pid_t input_pid;
    const char *name = "input";

    printf("여기서 input 프로세스를 생성합니다.\n");
    input_pid = fork();

    if(input_pid == 0)
    {
        prctl(PR_SET_NAME, name, NULL, NULL, NULL);
        input();
    }

    return input_pid;
}
