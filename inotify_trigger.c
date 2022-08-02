#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>

void handle_events(int fd, int *wd) {
  char buf[sizeof(struct inotify_event)];
  ssize_t len = read(fd, buf, sizeof(struct inotify_event));
  if (len != sizeof(struct inotify_event)) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  struct inotify_event *event = (struct inotify_event *)buf;

  if (event->mask & IN_OPEN) {
    printf("IN_OPEN\n");
  }
  if (event->mask & IN_CLOSE_NOWRITE) {
    printf("IN_CLOSE_NOWRITE\n");
  }
  if (event->mask & IN_CLOSE_WRITE) {
    printf("IN_CLOSE_WRITE\n");
  }
}

int main(int argc, char *argv[]) {

  int fd = inotify_init1(IN_NONBLOCK);
  if (fd == -1) {
    perror("inotify_init1");
    exit(EXIT_FAILURE);
  }

  int *wd = malloc(argc * sizeof(int));
  if (wd == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  for (int i = 1; i < argc; i++) {
    wd[i] = inotify_add_watch(fd, argv[i], IN_OPEN | IN_CLOSE);
    if (wd[i] == -1) {
      perror("inotify_add_watch");
      exit(EXIT_FAILURE);
    }
  }

  struct pollfd fds[2];

  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  fds[1].fd = fd;
  fds[1].events = POLLIN;

  while (1) {
    int poll_num = poll(fds, 2, -1);
    if (poll_num == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    }

    if (poll_num > 0) {
      if (fds[0].revents & POLLIN) {
        printf("Hello, World!\n");
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
