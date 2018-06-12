# snooplib
Easily list all files accessed by any command using a tiny shared library (<250 LOC) and no dependencies.

## Quick-start
 - Build the library (`make` should result in a `snooplib.so`)
 - Copy `snooplib.so` to somewhere convenient like `/usr/local/lib`
 - Set the `SNOOPLIB_OUTPUT_PATH` to an absolute path which a list of files will be written.
 - Run a binary with `LD_PRELOAD=/usr/local/lib/snooplib.so <binary>`
 - After the binary exits, the file at `SNOOPLIB_OUTPUT_PATH` will have a list of paths.
 
