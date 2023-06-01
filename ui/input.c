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

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

#define TOY_TOK_BUFSIZE 64
#define TOY_TOK_DELIM " \t\r\n\a"
#define TOY_BUFFSIZE 1024

typedef struct _sig_ucontext {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
} sig_ucontext_t;

static pthread_mutex_t global_message_mutex  = PTHREAD_MUTEX_INITIALIZER;

// global_message <~ 모든 문제를 만드는 전역 변수
static char global_message[TOY_BUFFSIZE];

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
    char *s = arg;

    printf("%s", s);

    while (1) {
        i = 0;
        // 여기서 뮤텍스
        // 과제를 억지로 만들기 위해 한 글자씩 출력 후 슬립
        pthread_mutex_lock(&global_message_mutex);
        while (global_message[i] != NULL) {
            printf("%c", global_message[i]);
            fflush(stdout);
            posix_sleep_ms(500);
            i++;
        }
        pthread_mutex_unlock(&global_message_mutex);
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
int toy_exit(char **args);

char *builtin_str[] = {
    "send",
    "mu",
    "sh",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &toy_send,
    &toy_mutex,
    &toy_shell,
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
    // 여기서 뮤텍스
    pthread_mutex_lock(&global_message_mutex);
    strcpy(global_message, args[1]);
    pthread_mutex_unlock(&global_message_mutex);
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
    } else if (pid < 0) {
        perror("toy");
    } else
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
        pthread_mutex_lock(&global_message_mutex);
        printf("TOY>");
        pthread_mutex_unlock(&global_message_mutex);
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

static void *
simple_thread(void* arg)
{
    char *s = arg;
    
    printf("%s\n", s);

    while(1)
    {
        sleep(1);
    }
}

int input()
{
    printf("나 input 프로세스!\n");

    struct sigaction sa;
    memset(&sa, 0, sizeof(sigaction));
    sigemptyset(&sa.sa_mask);

    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sa.sa_sigaction = segfault_handler;

    // input 프로세스에 seg_fault 시그널이 왔을 때 위의 코드를 실행
    sigaction(SIGSEGV, &sa, NULL); /* ignore whether it works or not */

    pthread_t command_thread_tid, sensor_thread_tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&command_thread_tid, &attr, command_thread, "command thread initiated");
    pthread_create(&sensor_thread_tid, &attr, simple_thread, "sensor thread initiated");


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
