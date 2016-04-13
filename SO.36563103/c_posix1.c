#define _XOPEN_SOURCE 500

#include <assert.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_LEN 1024

#ifdef SIGDET
#if SIGDET == 1
int isSignal = 1;       /*Termination detected by signals*/
#endif
#endif /* SIGDET */

extern char *strdup(const char *);

#ifndef SIGDET
int isSignal = 0;
#endif
/* One way to create a pipeline of N processes */

/* stderr.h */
#ifndef STDERR_H_INCLUDED
#define STDERR_H_INCLUDED

static void err_setarg0(const char *argv0);
static void err_sysexit(char const *fmt, ...);
static void err_syswarn(char const *fmt, ...);

#endif /* STDERR_H_INCLUDED */

typedef int Pipe[2];

/* exec_nth_command() and exec_pipe_command() are mutually recursive */
static void exec_pipe_command(int ncmds, char ***cmds, Pipe output);

/* With the standard output plumbing sorted, execute Nth command */
static void exec_nth_command(int ncmds, char ***cmds)
{
    assert(ncmds >= 1);
    if (ncmds > 1)
    {
        pid_t pid;
        Pipe input;
        if (pipe(input) != 0)
            err_sysexit("Failed to create pipe");
        if ((pid = fork()) < 0)
            err_sysexit("Failed to fork");
        if (pid == 0)
        {
            /* Child */
            exec_pipe_command(ncmds - 1, cmds, input);
        }
        /* Fix standard input to read end of pipe */
        dup2(input[0], 0);
        close(input[0]);
        close(input[1]);
    }
    execvp(cmds[ncmds - 1][0], cmds[ncmds - 1]);
    err_sysexit("Failed to exec %s", cmds[ncmds - 1][0]);
    /*NOTREACHED*/
}

/* Given pipe, plumb it to standard output, then execute Nth command */
static void exec_pipe_command(int ncmds, char ***cmds, Pipe output)
{
    assert(ncmds >= 1);
    /* Fix stdout to write end of pipe */
    dup2(output[1], 1);
    close(output[0]);
    close(output[1]);
    exec_nth_command(ncmds, cmds);
}

/* Execute the N commands in the pipeline */
static void exec_pipeline(int ncmds, char ***cmds)
{
    pid_t pid;
    assert(ncmds >= 1);

    if ((pid = fork()) < 0)
        err_syswarn("Failed to fork");
    if (pid != 0)
        return;
    exec_nth_command(ncmds, cmds);
}

/* Collect dead children until there are none left */
static void corpse_collector(void)
{
    pid_t parent = getpid();
    pid_t corpse;
    int status;
    while ((corpse = waitpid(0, &status, 0)) != -1)
    {
        fprintf(stderr, "%d: child %d status 0x%.4X\n",
                (int)parent, (int)corpse, status);
    }
}

/*  who | awk '{print $1}' | sort | uniq -c | sort -n */

static void dump_argv(const char *tag, int argc, char **argv)
{
    assert(argv != 0 && tag != 0);
    printf("%s: (%d)", tag, argc);
    for (int i = 0; i < argc; i++)
        printf(" {%s}", (argv[i] == NULL) ? "(null)" : argv[i]);
    putchar('\n');
    fflush(stdout);
    assert(argv[argc] == 0);
}

static void dump_pipeline(int cmdn, char ***cmdv)
{
    printf("SOC\n");
    for (int i = 0; i < cmdn; i++)
    {
        printf("cmd [%d]", i);
        char **args = cmdv[i];
        while (*args != 0)
            printf(" {%s}", *args++);
        putchar('\n');
    }
    printf("EOC\n");
    fflush(stdout);
}

static void exec_arguments(int argc, char **argv)
{
    dump_argv("Entering exec_arguments", argc, argv);
    /* Split the command line into sequences of arguments */
    /* Break at pipe symbols as arguments on their own */

    char ***cmdv = malloc(sizeof *cmdv * (argc / 2));
    char **args = malloc(sizeof *args * (argc + 1));
    int cmdn = 0;
    int argn = 0;

    cmdv[cmdn++] = &args[argn];

    for (int i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        //printf("Processing %d [%s]\n", i, arg);
        if (strcmp(arg, "|") == 0)
        {
            if (i == 1)
                err_sysexit("Syntax error: pipe before any command");
            if (args[argn - 1] == 0)
                err_sysexit("Syntax error: two pipes with no command between");
            arg = 0;
        }
        args[argn++] = arg;
        if (arg == 0)
            cmdv[cmdn++] = &args[argn];
    }
    if (args[argn - 1] == 0)
        err_sysexit("Syntax error: pipe with no command following");
    args[argn] = 0;

    dump_pipeline(cmdn, cmdv);
    exec_pipeline(cmdn, cmdv);
    free(cmdv);
    free(args);
}

static const char *arg0 = "<undefined>";

static void err_setarg0(const char *argv0)
{
    arg0 = argv0;
}

static void err_vsyswarn(char const *fmt, va_list args)
{
    int errnum = errno;
    fprintf(stderr, "%s:%d: ", arg0, (int)getpid());
    vfprintf(stderr, fmt, args);
    if (errnum != 0)
        fprintf(stderr, " (%d: %s)", errnum, strerror(errnum));
    putc('\n', stderr);
}

static void err_syswarn(char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    err_vsyswarn(fmt, args);
    va_end(args);
}

static void err_sysexit(char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    err_vsyswarn(fmt, args);
    va_end(args);
    exit(1);
}

pid_t foreground = -1;

struct command
{
    char *const *argv;
};

/* Helper function that determines the beginning of a string */
static int StartsWith(const char *a, const char *b)
{
    if (strncmp(a, b, strlen(b)) == 0)
        return 1;
    return 0;
}

/* Remove zombie processes */
/* Return if background process terminated */
/* http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/signal.h.html */
static
void Janitor(int status)
{
    if (status == SIGCHLD)     /*Child process terminated, stopped, or continued*/
    {
        int a = 1;
        while (a)
        {
            pid_t pid_my1 = waitpid(-1, &status, WNOHANG);
            /*WNOHANG = return immediately if no child has exited*/
            /*Wait*/
            /*http://linux.die.net/man/2/waitpid*/

            if (0 < pid_my1) /*Still things to clean up*/
            {
                if (pid_my1 != foreground) /*Don't stop me since it's the foregound process*/

                {
                    /*http://linux.die.net/man/3/wait*/
                    if (WIFSIGNALED(status) || WIFEXITED(status))        /*Child process terminated*/
                    {
                        printf("\n\n\n\n%d terminated \n\n\n\n\n", pid_my1);
                    }
                }
            }
            else    /*All work done, for now*/
            {
                a = 0;
            }
        }
    }
}

/* helper function that removes spaces from a string */
static
void RemoveSpaces(char *source)
{
    char *i = source;
    char *j = source;
    while (*j != 0)
    {
        *i = *j++;
        if (*i != ' ')
            i++;
    }
    *i = 0;
}

/* Signal handler */
static void sighandler(int signo)
{
    assert(signo == SIGINT);
    write(2, "Received SIGINT\n", 16);
}

int main(int argc, char *argv[])
{
    char *p;
    char *array[40];
    char line[BUFFER_LEN];
    char line2[BUFFER_LEN];
    char *argv2[100];
    int argc2 = 0;
    size_t length;
    int i = 0;
    int isBackground = 0;
    int built_in_command = 0;
    int fd[2];
    long time;
    int status = 0;
    int max = 80;
    int b;
    struct timeval time_start;
    struct timeval time_end;
    sigset_t my_sig;
    pid_t pid_temp;
    char *pathValue;
    struct sigaction sa, osa;

    memset(&sa, '\0', sizeof(sa));

    pid_temp = 0; /* To please the compiler */
    sa.sa_handler = sighandler;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, &osa);

    /* get the PATH environment to find if less is installed */
    pathValue = getenv("PATH");
    if (!pathValue)
    {
        printf("'%s' is not set.\n", "PATH");
    }
    else
    {
        printf("'%s' is set to %s.\n", "PATH", pathValue);
    }

    while (1)
    {
        i = 0;

        Janitor(SIGCHLD);

        printf("miniShell>> ");
        memset(line, 0, sizeof line); /*Reset*/
        if (!fgets(line, BUFFER_LEN, stdin))
        {
            putchar('\n');
            break;
        }
        Janitor(SIGCHLD);
        strncpy(line2, line, BUFFER_LEN);
        RemoveSpaces(line2);
        if (StartsWith(line2, "\n"))
        {
            continue;
        }
        length = strlen(line);
        if (line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        if (0 == built_in_command)     /*Not a built in command, so let execute it*/
        {
            argv2[i] = NULL;
            argc = i;
            for (i = 0; i < argc2; i++)
            {
                printf("%s\n", argv2[i]);
            }

            isBackground = 0;

            for (b = 0; b < max; b++)
            {
                if ('&' == line[b])
                {
                    isBackground = 1;
                }
            }

            if (isBackground == 1)      /*If backgroundprocess*/
            {
                if (pipe(fd) == -1)      /*(two new file descriptors)*/
                {
                    perror("Failed creating pipe\n");
                }

                pid_temp = fork();
            }
            else if (isBackground == 0)     /*If foreground process*/
            {
                gettimeofday(&time_start, NULL);

                if (1 == isSignal)      /*If using signaldetection*/
                {
                    sigemptyset(&my_sig); /*empty and initialising a signal set*/
                    sigaddset(&my_sig, SIGCHLD);    /*Adds signal to a signal set (my_sig)*/
                    /*http://pubs.opengroup.org/onlinepubs/7908799/xsh/sigprocmask.html*/
                    sigprocmask(SIG_BLOCK, &my_sig, NULL);
                }

                /*pid_temp = fork();*/
                foreground = pid_temp;  /*Set pid for foreground process*/
            }
            err_setarg0(argv[argc-argc]);

            printf("Command line: %s\n", line);
            argv2[1] = line;
            i = 1;
            p = strtok(line, " ");

            array[0] = NULL;
            while (p != NULL)
            {
                array[i++] = p;
                p = strtok(NULL, " ");
            }
            array[i] = NULL;
            dump_argv("Before exec_arguments", i, array);
            exec_arguments(i, array);
            corpse_collector();

            if (0 < pid_temp)
            {
                /*Parent process*/
            }
            else if (0 > pid_temp)
            {
                /*Error*/
            }

            else
            {
                /*Child process*/
                if (1 == isBackground)      /*Backgroundprocess*/
                {
                    dup2(fd[STDIN_FILENO], STDIN_FILENO);
                    close(fd[0]);
                    close(fd[1]);
                }
                /*http://www.lehman.cuny.edu/cgi-bin/man-cgi?execvp+2*/
                /*if (execvp(argv2[0],argv2) < 0)	{
                   //     printf("We are sorry to inform you that something went wrong %d \n", errno);
                   }*/
            }
            if (0 == isBackground)      /*Foregroundprocess*/
            {
                waitpid(foreground, &status, 0);    /*Waiting*/
                /*Foregroundprocess terminated*/

                gettimeofday(&time_end, NULL);
                time = (time_end.tv_sec - time_start.tv_sec) * 1000000 +
                       time_end.tv_usec - time_start.tv_usec;
                printf("Execution time %ld.%03ld ms\n", time / 1000, time % 1000);

                if (1 == isSignal)      /*If using signaldetection*/
                {
                    int a = sigprocmask(SIG_UNBLOCK, &my_sig, NULL);
                    /*http://man7.org/linux/man-pages/man2/sigprocmask.2.html*/
                    if (0 == a)
                    {
                        /*Sigprocmask was successfull*/
                    }
                    else
                    {
                        /*Sigprocmask was not successfull, return=-1*/
                    }
                    Janitor(SIGCHLD);
                }
            }
            else if (1 == isBackground)
            {
                close(fd[0]);
                close(fd[1]);
            }
        }

        built_in_command = 0;   /*Reset*/
        memset(line, 0, sizeof line); /*Reset*/
    }

    return(0);
}
