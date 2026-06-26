## Building
Building the default package with sending functionality:
```console
$ nix build .
```
And you can remove sending functionality with package no-sender:
```console
$ nix build .#no-sender
```
## Installation
Install with sender.
```console
$ nix profile install git+https://gitlab.com/snipx-project/snipx-record
```
Install without sender.
```console
$ nix profile install git+https://gitlab.com/snipx-project/snipx-record#no-sender
```
