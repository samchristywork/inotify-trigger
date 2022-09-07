#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

#define REPORT_EVENT(x)                     \
  {                                         \
    if (event->mask & x) {                  \
      if (verbose) {                        \
        fprintf(stdout, "Event: " #x "\n"); \
      }                                     \
    }                                       \
  }

char *separator = "-----SEPARATOR-----";
int quiet = 0;
char *command = NULL;
char *shell = "/usr/bin/sh";
double debounce = 0.5;
int running = 1;

int fd = 0;
int *wd = 0;

struct pthread_info {
  useconds_t timeout;
};

void task() {
  static struct timespec oldTime = {0};
  struct timespec newTime;
  clock_gettime(CLOCK_REALTIME, &newTime);
  double old = oldTime.tv_sec + (double)oldTime.tv_nsec / 1000000000.;
  double new = newTime.tv_sec + (double)newTime.tv_nsec / 1000000000.;
  double difference = new - old;

  if (difference < debounce) {
    if (quiet == 0) {
      fprintf(stderr, "Hit debounce. Disable with -d0 or suppress this message with -q.\n");
    }
    return;
  }

  oldTime = newTime;

  puts(separator);
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
  return NULL;
}

void reload_watches(int argc, char *argv[], int verbose) {
  for (int i = 0; i < argc - optind; i++) {
    inotify_rm_watch(fd, wd[i]);
  }

  if (optind < argc) {
    int i = optind;
    while (i < argc) {
      wd[i] = inotify_add_watch(fd, argv[i], IN_OPEN | IN_CLOSE);
      if (verbose) {
        fprintf(stdout, "Loading %s\n", argv[i]);
      }
      if (wd[i] == -1) {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
      }
      i++;
    }
  }
}

void handle_events(int fd, int *wd, int argc, char *argv[], int verbose) {
  char buf[sizeof(struct inotify_event)];
  ssize_t len = read(fd, buf, sizeof(struct inotify_event));
  if (len != sizeof(struct inotify_event)) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  struct inotify_event *event = (struct inotify_event *)buf;

  //REPORT_EVENT(IN_ACCESS)
  //REPORT_EVENT(IN_ATTRIB)
  //REPORT_EVENT(IN_CLOSE)
  //REPORT_EVENT(IN_CLOSE_NOWRITE)
  REPORT_EVENT(IN_CLOSE_WRITE)
  REPORT_EVENT(IN_CREATE)
  REPORT_EVENT(IN_DELETE)
  REPORT_EVENT(IN_DELETE_SELF)
  REPORT_EVENT(IN_MODIFY)
  REPORT_EVENT(IN_MOVE)
  REPORT_EVENT(IN_MOVED_FROM)
  REPORT_EVENT(IN_MOVE_SELF)
  //REPORT_EVENT(IN_OPEN)

  if (event->mask & IN_IGNORED) {
    if (verbose) {
      fprintf(stdout, "IN_IGNORED\n");
    }
    reload_watches(argc, argv, verbose);
  }
  fflush(stdout);
}

void usage(char *argv[]) {
  fprintf(stderr,
          "Usage: %s [-c command] [-r milliseconds] [-s shell] [file(s)]\n"
          " -c,--command   Command to run (string).\n"
          " -d,--debounce  Debounce period in milliseconds (default 100ms).\n"
          " -h,--help      Print this usage message.\n"
          " -p,--separator The string to use to separate commands.\n"
          " -q,--quiet     Don't spawn unnecessary messages.\n"
          " -r,--repeat    Period to repeat the command in milliseconds.\n"
          " -s,--shell     Specifies the shell to be used (default /usr/bin/sh).\n"
          " -v,--verbose   Display additional logging information.\n"
          "",
          argv[0]);
  exit(EXIT_FAILURE);
}

useconds_t string_to_us(char *str) {
  useconds_t value = atoi(str);
  char *unit;
  for (unit = str;; unit++) {
    if (isalpha(unit[0]) || unit[0] == 0) {
      break;
    }
  }
  useconds_t multiplier = 1;
  if (strcmp(unit, "us") == 0) {
    multiplier = 1;
  }
  if (strcmp(unit, "ms") == 0) {
    multiplier = 1000;
  }
  if (strcmp(unit, "s") == 0) {
    multiplier = 1000 * 1000;
  }
  if (strcmp(unit, "m") == 0) {
    multiplier = 1000 * 1000 * 60;
  }
  return value * multiplier;
}

void sig_term_handler(int signum, siginfo_t *info, void *ptr) {
  fprintf(stderr, "Received SIGINT, exiting.\n");
  running = 0;
}

int main(int argc, char *argv[]) {

  struct sigaction act;
  bzero(&act, sizeof(struct sigaction));
  act.sa_sigaction = sig_term_handler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGINT, &act, NULL);

  useconds_t refresh = 0;
  int verbose = 0;

  int opt;
  int option_index = 0;
  char *optstring = "c:d:hp:qr:s:v";
  static struct option long_options[] = {
      {"command", required_argument, 0, 'c'},
      {"debounce", required_argument, 0, 'd'},
      {"help", no_argument, 0, 'h'},
      {"separator", required_argument, 0, 'p'},
      {"quiet", no_argument, 0, 'q'},
      {"repeat", required_argument, 0, 'r'},
      {"shell", required_argument, 0, 's'},
      {"verbose", no_argument, 0, 'v'},
      {0, 0, 0, 0},
  };
  while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
    if (opt == 'c') {
      command = malloc(strlen(optarg) + 1);
      if (command == NULL) {
        perror("malloc");
        usage(argv);
      }
      strcpy(command, optarg);
    } else if (opt == 'd') {
      debounce = (float)string_to_us(optarg) / 1000000.;
    } else if (opt == 'h') {
      usage(argv);
    } else if (opt == 'p') {
      separator = malloc(strlen(optarg) + 1);
      strcpy(separator, optarg);
    } else if (opt == 'q') {
      quiet = 1;
    } else if (opt == 'r') {
      refresh = string_to_us(optarg);
    } else if (opt == 's') {
      shell = malloc(strlen(optarg));
      if (shell == NULL) {
        perror("malloc");
        usage(argv);
      }
      strcpy(shell, optarg);
    } else if (opt == 'v') {
      verbose = 1;
    } else if (opt == '?') {
      usage(argv);
    } else {
      puts(optarg);
    }
  }

  if (verbose) {
    fprintf(stderr, "Initializing inotify.\n");
  }
  fd = inotify_init1(IN_NONBLOCK);
  if (fd == -1) {
    perror("inotify_init1");
    usage(argv);
  }

  wd = malloc((argc - optind + 4) * sizeof(int));
  if (wd == NULL) {
    perror("malloc");
    usage(argv);
  }
  bzero(wd, (argc - optind) * sizeof(int));

  if (verbose) {
    fprintf(stderr, "Loading inotify watches.\n");
  }
  reload_watches(argc, argv, verbose);

  struct pollfd fds[2];

  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  fds[1].fd = fd;
  fds[1].events = POLLIN;

  if (refresh) {
    if (verbose) {
      fprintf(stderr, "Setting refresh task in pthread.\n");
    }
    pthread_t thread;
    struct pthread_info tinfo;
    tinfo.timeout = refresh;
    pthread_create(&thread, NULL, periodic_task, &tinfo);
  }

  if (verbose) {
    fprintf(stderr, "Entering main loop.\n");
  }
  while (1) {
    int poll_num = poll(fds, 2, -1);
    if (poll_num == -1) {
      if (!running) {
        break;
      }
      perror("poll");
      exit(EXIT_FAILURE);
    }

    if (poll_num > 0) {
      if (fds[0].revents & POLLIN) {
        if (verbose) {
          fprintf(stderr, "Got user input.\n");
        }
        task();
        char buf[1];
        while (read(fds[0].fd, buf, 1) > 0 && buf[0] != '\n') {
        }
      }

      if (verbose) {
        fprintf(stderr, "Inotify event detected.\n");
      }
      if (fds[1].revents & POLLIN) {
        handle_events(fd, wd, argc, argv, verbose);
      }
    }
  }

  if (verbose) {
    fprintf(stderr, "Cleaning up.\n");
  }
  if (command) {
    free(command);
  }
  close(fd);
  free(wd);
  exit(EXIT_SUCCESS);
}
