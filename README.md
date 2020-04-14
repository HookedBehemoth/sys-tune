# sys-tune
## Background audio player for the Nintendo switch + Tesla overlay

## Installation
1. Download the release zip from [here](https://github.com/HookedBehemoth/sys-tune/releases/latest)
2. Extract the zip to the root of your sd card.
3. Put mp3 files to your sd card.

You can manage playback via the Tesla overlay in the release.

## Screenshots
![Main](/sample/libtesla_1586882452.jpg)
![Main](/sample/libtesla_1586882672.jpg)
![Main](/sample/libtesla_1586882735.jpg)
(Alpha values are wrong in these screenshots. The overlay will be less transparent.)

## General information
### Other audio formats
While I'd like to add flac this also bloats executable size by a lot and I don't want to hog all the memory.
### Sleep mode
Sadly putting the console to sleep while music is playing causes issues. So please pause before you put your console to sleep.

## Info for developers
I implemented an IPC interface accessible via service wrappers [here](/ipc/).

My [Tesla overlay](/overlay/source/) uses these bindings.