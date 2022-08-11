CC := gcc

LIBS := -lpthread
PREFIX = /usr/local

all: build/inotify_trigger

build/inotify_trigger: src/inotify_trigger.c
	mkdir -p build/
	${CC} $^ -o $@ ${LIBS}

install: build/inotify_trigger
	@echo "Installing inotify-trigger."
	mkdir -p $(PREFIX)/bin
	cp build/inotify_trigger $(PREFIX)/bin
	chmod 755 $(PREFIX)/bin/inotify_trigger

clean:
	rm -rf build/

.PHONY: clean install
