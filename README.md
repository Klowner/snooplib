# snooplib
Easily list all files accessed by any command using a tiny shared library (<250 LOC) and no dependencies.

## Quick-start
 - Build the library (`make` should result in a `snooplib.so`)
 - Copy `snooplib.so` to somewhere convenient like `/usr/local/lib`
 - Set the `SNOOPLIB_OUTPUT_PATH` to an absolute path which a list of files will be written.
 - Run a binary with `LD_PRELOAD=/usr/local/lib/snooplib.so <binary>`
 - After the binary exits, the file at `SNOOPLIB_OUTPUT_PATH` will have a list of paths.
 
## Example
```sh
> SNOOPLIB_OUTPUT_PATH=$(pwd)/manifest LD_PRELOAD=/home/mark/storage/code/cfpackbot/tools/snooplib/snooplib.so nmap -p 22 127.0.0.1
Starting Nmap 7.70 ( https://nmap.org ) at 2018-06-12 14:44 CDT
Nmap scan report for localhost.localdomain (127.0.0.1)
Host is up (0.000088s latency).

PORT   STATE SERVICE
22/tcp open  ssh

Nmap done: 1 IP address (1 host up) scanned in 0.03 seconds
> cat manifest
/dev/urandom
/dev/tty
```
As you can see above, `nmap` accessed `/dev/urandom` and `/dev/tty`. 👍
