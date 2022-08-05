#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

#define FOO(x)             \
  {                        \
    if (event->mask & x) { \
      printf(#x "\n");     \
    }                      \
  }

char *command = NULL;
char *shell = "/usr/bin/sh";
double debounce = 0.1;

int fd;
int *wd;

struct pthread_info {
  int timeout;
};

void task() {
  static struct timespec oldTime = {0};
  struct timespec newTime;
  clock_gettime(CLOCK_REALTIME, &newTime);
  double old = oldTime.tv_sec + (double)oldTime.tv_nsec / 1000000000.;
  double new = newTime.tv_sec + (double)newTime.tv_nsec / 1000000000.;
  double difference = new - old;

  if (difference < debounce) {
    return;
  }

  oldTime = newTime;

  if (command) {
    pid_t pid = fork();
    if (pid == 0) {
      int ret = execl(shell, shell, "-c", command, (char *)NULL);
      if (ret == -1) {
        perror("execl");
      }
    }
  }
}

static void *periodic_task(void *arg) {
  struct pthread_info *tinfo = arg;
  while (1) {
    task();
    usleep(tinfo->timeout);
  }
}

void reload_watches(int argc, char *argv[]) {
  for (int i = 0; i < argc - optind; i++) {
    inotify_rm_watch(fd, wd[i]);
  }

  if (optind < argc) {
    int i = optind;
    while (i < argc) {
      wd[i] = inotify_add_watch(fd, argv[i], IN_OPEN | IN_CLOSE);
      printf("Loading %s\n", argv[i]);
      if (wd[i] == -1) {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
      }
      i++;
    }
  }
}

void handle_events(int fd, int *wd, int argc, char *argv[]) {
  char buf[sizeof(struct inotify_event)];
  ssize_t len = read(fd, buf, sizeof(struct inotify_event));
  if (len != sizeof(struct inotify_event)) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  struct inotify_event *event = (struct inotify_event *)buf;

  FOO(IN_ACCESS)
  FOO(IN_ATTRIB)
  FOO(IN_CLOSE)
  FOO(IN_CLOSE_NOWRITE)
  FOO(IN_CLOSE_WRITE)
  FOO(IN_CREATE)
  FOO(IN_DELETE)
  FOO(IN_DELETE_SELF)
  FOO(IN_MODIFY)
  FOO(IN_MOVE)
  FOO(IN_MOVED_FROM)
  FOO(IN_MOVE_SELF)
  FOO(IN_OPEN)

  task();

  if (event->mask & IN_IGNORED) {
    printf("IN_IGNORED\n");
    reload_watches(argc, argv);
  }
  fflush(stdout);
}

void usage(char *argv[]) {
  fprintf(stderr,
          "Usage: %s [-c command] [-r milliseconds] [-s shell] [file(s)]\n"
          " -c,--command   Command to run (string).\n"
          " -d,--debounce  Debounce period in milliseconds (default 100ms).\n"
          " -h,--help      Print this usage message.\n"
          " -r,--repeat    Period to repeat the command in milliseconds.\n"
          " -s,--shell     Specifies the shell to be used (default /usr/bin/sh).\n"
          "",
          argv[0]);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

  int refresh = 0;

  int opt;
  int option_index = 0;
  char *optstring = "c:d:hr:s:";
  static struct option long_options[] = {
      {"command", required_argument, 0, 'c'},
      {"debounce", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {"repeat", required_argument, 0, 'r'},
      {"shell", required_argument, 0, 's'},
      {0, 0, 0, 0},
  };
  while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
    if (optopt == 0) {
      usage(argv);
    }
    if (opt == 'c') {
      command = malloc(strlen(optarg));
      if (command == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
      }
      strcpy(command, optarg);
    } else if (opt == 'd') {
      debounce = (float)atoi(optarg) / 1000.;
    } else if (opt == 'h') {
      usage(argv);
    } else if (opt == 'r') {
      refresh = atoi(optarg);
    } else if (opt == 's') {
      shell = malloc(strlen(optarg));
      if (shell == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
      }
      strcpy(shell, optarg);
    } else {
      puts(optarg);
    }
  }

  fd = inotify_init1(IN_NONBLOCK);
  if (fd == -1) {
    perror("inotify_init1");
    exit(EXIT_FAILURE);
  }

  wd = malloc((argc - optind) * sizeof(int));
  if (wd == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  reload_watches(argc, argv);

  struct pollfd fds[2];

  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  fds[1].fd = fd;
  fds[1].events = POLLIN;

  if (refresh) {
    pthread_t thread;
    struct pthread_info tinfo;
    tinfo.timeout = refresh;
    pthread_create(&thread, NULL, periodic_task, &tinfo);
  }

  while (1) {
    int poll_num = poll(fds, 2, -1);
    if (poll_num == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    }

    if (poll_num > 0) {
      if (fds[0].revents & POLLIN) {
        task();
        char buf[1];
        while (read(fds[0].fd, buf, 1) > 0 && buf[0] != '\n') {
        }
      }

      if (fds[1].revents & POLLIN) {
        handle_events(fd, wd, argc, argv);
      }
    }
  }

  close(fd);
  free(wd);
  exit(EXIT_SUCCESS);
}
