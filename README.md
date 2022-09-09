![Banner](https://s-christy.com/status-banner-service/inotify-trigger/banner-slim.svg)

## Overview

Inotify is a Linux API for monitoring file-system events. This program leverages
the Inotify API to run commands when events happen, which can be used to
automatically re-compile programs when the source code is changed, reload
services when config files are saved, or many other use cases.

## Example

In this example, the program monitors the file `src/main.c` and compiles and
runs the code when the file is saved to disk.

```
inotify_trigger -c "cd src/ && gcc *.c -o main -lm && ./main" src/main.c
```

## Usage

## Dependencies

## License

This work is licensed under the GNU General Public License version 3 (GPLv3).

[<img src="https://s-christy.com/status-banner-service/GPLv3_Logo.svg" width="150" />](https://www.gnu.org/licenses/gpl-3.0.en.html)

TODO Image here
