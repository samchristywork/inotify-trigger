CC := gcc

LIBS := -lpthread

all: build/inotify_trigger

build/inotify_trigger: inotify_trigger.c
	mkdir -p build/
	${CC} $^ -o $@ ${LIBS}

clean:
	rm -rf build/

.PHONY: clean