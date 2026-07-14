> 🚨 Currently in early development stage.
# SnipX
SnipX is an utility for recording instant replays on X11, sending and storing them on autonomous server. Inspired by [medal.tv](https://medal.tv)
## Requirements
- X11 server
- libXinerama
- libXext
- Go (for sending, [can be disabled](##Building))
- PulseAudio
- glib (yet)
- libpixbuf (yet)
- libnotify (yet)
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
## Setting audio sources
To record audio, select the correct sources for your microphone and desktop audio. You can either pass their indices using the `--desktop` and `--mic` flags or configure them in `pavucontrol`.

If you don't specify a microphone source, microphone audio will not be recorded.

To list all available audio sources, run:

```sh
$ snipx --sources
```

Example output:

```text
    64    Monitor of ROUTIST R2 Analog Surround 4.0
    65    ROUTIST R2 Analog Surround 4.0
    66    USB PnP Audio Device Mono
    67    Monitor of Built-in Audio Digital Stereo (IEC958)
    68    Built-in Audio Analog Stereo
    127   Monitor of TU106 High Definition Audio Controller Digital Stereo (HDMI)
    136   Monitor of Built-in Audio Digital Stereo (HDMI)
```

Sources whose names begin with **"Monitor of"** are desktop (system) audio sources, while the others are input devices such as microphones.

Pass the corresponding indices to `--desktop` and `--mic`. For example:

```sh
$ snipx --desktop 64 --mic 66
```
## Logging
By default, snipx saves logs in `$HOME/.local/share/snipx/logs/`, but it can be changed with environmental variable `SNIPX_LOG_DIR`.
## Env variables
- `SNIPX_DIR` - snipx temp and log directory.
- `SNIPX_TMP_DIR` - snipx temp directory.
- `SNIPX_LOG_DIR` - snipx log directory.
## TODO
- [x] Add --help (-h) flag
- [x] Add --length (-l) flag
- [x] Add saving files locally
- [x] Connect sender.
- [x] Add saving files locally until the server is available
- [x] Add capturing the screen via shared memory
- [x] Replace simple pulseaudio API with stream API.
- [x] Add support of second audio stream.
- [x] Add list of available sound monitors.
- [x] Add deferred render.
- [x] Add output flag
- [ ] Add real-time video compression
- [ ] Port to wayland
