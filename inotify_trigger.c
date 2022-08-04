#include <errno.h>
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

struct pthread_info {
  int timeout;
};

void task() {
  if (command) {
    pid_t pid = fork();
    if(pid==0){
      int ret = execl(shell, shell, "-c", command, (char *)NULL);
      if(ret==-1){
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

void reload_watches() { printf("stub\n"); }

void handle_events(int fd, int *wd) {
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

  if (event->mask & IN_IGNORED) {
    printf("IN_IGNORED\n");
    reload_watches();
  }
  fflush(stdout);
}

void usage(char *argv[]) {
  fprintf(stderr,
          "Usage: %s [-c command] [-r milliseconds] [file(s)]\n"
          " -c\tCommand to run (string).\n"
          " -h\tPrint this usage message.\n"
          " -r\tPeriod to repeat the command in milliseconds.\n"
          "",
          argv[0]);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

  int refresh = 0;

  int opt;
  char *optstring = "c:hr:";
  while ((opt = getopt(argc, argv, optstring)) != -1) {
    if (opt == 'c') {
      command = malloc(strlen(optarg));
      if (command == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
      }
      strcpy(command, optarg);
    } else if (opt == 'h') {
      usage(argv);
    } else if (opt == 'r') {
      refresh = atoi(optarg);
    } else {
      puts(optarg);
    }
  }

  int fd = inotify_init1(IN_NONBLOCK);
  if (fd == -1) {
    perror("inotify_init1");
    exit(EXIT_FAILURE);
  }

  int *wd = malloc((argc - optind) * sizeof(int));
  if (wd == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  if (optind < argc) {
    int i = 0;
    while (optind < argc) {
      wd[i] = inotify_add_watch(fd, argv[optind++], IN_OPEN | IN_CLOSE);
      if (wd[i] == -1) {
        perror("inotify_add_watch");
        exit(EXIT_FAILURE);
      }
      i++;
    }
    printf("\n");
  }

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
        handle_events(fd, wd);
      }
    }
  }

  close(fd);
  free(wd);
  exit(EXIT_SUCCESS);
}
