> 🚨 Currently in early development stage.
# SnipX
SnipX is an utility for recording instant replays on X11, sending and storing them on autonomous server. Inspired by [medal.tv](https://medal.tv)
## Requirements
- X11 server
- PulseAudio
- A lot of free space in RAM (yet)
## Building
```console
$ make
```
Or you can compile without `sending` functionality.
```console
$ SENDER=false make
```
## Installation
```console
$ make install
```
## Usage
Starting recording.
```console
$ snipx
```
Saving replay.
```console
$ snipx -b
```
## Logging
By default, snipx saves logs in `$HOME/.local/share/snipx/logs/`, but it can be changed with environmental variable `SNIPX_LOG_DIR`.
## Env variables
- `SNIPX_DIR` - snipx temp and log directory.
- `SNIPX_TMP_DIR` - snipx temp directory.
- `SNIPX_LOG_DIR` - snipx log directory.
- `SNIPX_OUT_DIR` - video output directory.
## TODO
- [x] Add --help (-h) flag
- [x] Add --length (-l) flag
- [x] Add saving files locally
- [x] Connect sender.
- [x] Add saving files locally until the server is available
- [ ] Add capturing the screen via shared memory
- [ ] Add NVENC support
