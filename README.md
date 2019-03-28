# tvist
Automatic video acoustic image manipulation built in C, using ffmpeg and sox.

## Build
Run `make` to compile. Note that running requires sox and ffmpeg, and for them to be visible in your system's environment variables.

This almost certainly won't run on anything but Linux. It has only been tested on Arch with ffmpeg 4.1.2 and sox 14.4.2. If you want it on Windows or Mac, port it yourself.

## Usage
Run `tvist inputfile outputfile` to begin operation. The input and output file types must be those that ffmpeg are compatible with (eg mp4, avi, etc.). This process disregards any audio of the input file, you must re-add that yourself if you'd like; the `-map` flag in ffmpeg can do something like this.

In the current version you must change the command arguments for sox in tvist.c mutate() and recompile to change its options. In the future I plan to implement a system whereby these commands can be loaded from a list in a file; and later add functionality for variables of the command's arguments to smoothly oscillate between set values across the duration of the video. At some point I might refactor this program to allow direct image input & output (ie skipping the steps involving ffmpeg).

## Contributing
If you have problems with the program, fix them and put in a pull request. This program was built in a two days and the only motivation I have to maintain it is my own usage of it. If you want features added, either add them yourself or give me money to do it.
