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
## Usage
Starting recording.
```console
$ snipx
```
Saving replay.
```console
$ snipx -b
```
## TODO
- [x] Add --help (-h) flag
- [x] Add --length (-l) flag
- [ ] Add capturing the screen via shared memory
- [ ] Add NVENC support
