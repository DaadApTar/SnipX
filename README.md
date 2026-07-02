> 🚨 Currently in early development stage.
# SnipX
SnipX is an utility for recording instant replays on X11, sending and storing them on autonomous server. Inspired by [medal.tv](https://medal.tv)
## Requirements
- X11 server
- libXinerama
- libXext
- Go (for sending, [can be disabled](##Building))
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
For NixOS follow [NixOS build and install instructions](./NIXOS.md).
## Usage
Starting recording.
```console
$ snipx
```
Saving replay.
```console
$ snipx -b
```
## Setting sound monitors
To record audio, you need to select the correct audio sources for your microphone and desktop audio. You can do this either by using the `--desktop` and `--mic` flags or by configuring them in `pavucontrol`.

If you don't specify a microphone source, microphone audio will not be recorded.

To find the available sources from the command line, run:

```sh
$ pactl list sources short
```

The output will contain source names such as `alsa_input...` (microphones) and `alsa_output...monitor` (desktop/system audio). Pass these names to the `--mic` and `--desktop` flags respectively.
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
- [x] Add capturing the screen via shared memory
- [x] Replace simple pulseaudio API with stream API.
- [x] Add support of second audio stream.
- [ ] Add list of available sound monitors.
- [ ] Add polled render.
- [ ] Add real-time video compression
