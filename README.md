![Banner](https://s-christy.com/sbs/status-banner.svg?icon=action/update&hue=50&title=Inotify%20Trigger&description=A%20simple%20and%20reasonable%20alternative%20to%20entr)

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

Here is the usage information for this program:

```
Usage: inotify_trigger [-c command] [-r milliseconds] [-s shell] [file(s)]
 -c,--command   Command to run (string).
 -d,--debounce  Debounce period in milliseconds (default 100ms).
 -h,--help      Print this usage message.
 -p,--separator The string to use to separate commands.
 -q,--quiet     Don't spawn unnecessary messages.
 -r,--repeat    Period to repeat the command in milliseconds.
 -s,--shell     Specifies the shell to be used (default /usr/bin/sh).
 -v,--verbose   Display additional logging information.
```

## Dependencies

There are no dependencies besides a C compiler and Make. The Inotify API is part
of Linux and does not need a package to be used.

## Acknowledgements

This program is similar to, and in some cases modelled after, the programs
`inotifywait` and `entr`. I have used both of these programs extensively. This
repository is an attempt to build upon the strengths of both of these programs
and to add some features that I felt were sorely needed.

## License

This work is licensed under the GNU General Public License version 3 (GPLv3).

[<img src="https://s-christy.com/status-banner-service/GPLv3_Logo.svg" width="150" />](https://www.gnu.org/licenses/gpl-3.0.en.html)

TODO Image here
