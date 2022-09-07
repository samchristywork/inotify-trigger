CC := gcc

LIBS := -lpthread
CFLAGS := -g -Wall -Wpedantic

PREFIX = /usr/local

all: build/inotify_trigger

build/inotify_trigger: src/inotify_trigger.c
	mkdir -p build/
	${CC} ${CFLAGS} $^ -o $@ ${LIBS}

valgrind: all
	valgrind ./build/inotify_trigger -q -c "echo 'test'" Makefile

install: build/inotify_trigger
	@echo "Installing inotify-trigger."
	mkdir -p $(PREFIX)/bin
	cp build/inotify_trigger $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin/inotify_trigger

clean:
	rm -rf build/

.PHONY: clean install
