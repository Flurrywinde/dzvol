dzvol
=====

An extremely lightweight dzen2 volume monitor inspired by
[bruenig's post on the Arch forums](https://bbs.archlinux.org/viewtopic.php?id=46608).

Alsa mixer volume code obtained from
[this code](https://code.google.com/p/yjl/source/browse/Miscellaneous/get-volume.c).

## Flurrywinde's Mods

### Support volume over 100%
On my systems, 100% volume is still too low, so I often raise it above 100%. `dzvol`, however, would show anything over 100% as still just 100%. Thus, I added two new parameters, --pulseaudio and --max. The original code gets the volume from Alsa, but --pulseaudio calls pamixer to get it. Then, use --max to tell dzvol what you want it to use for its maximum (e.g. 200%), and it will show that as 100% on the slider.

#### TODO: Use the C library, libpulse, instead of popen'ing pamixer.
I thought making a system call would be slow. Besides, no C programmer worth their salt shells out if there's a low-level way to do the same thing! But then I benchmarked it, and wow, the popen call to pamixer is an order of magnitude faster than using the alsa library (on average, 3 ms vs 0.2 ms).

### Just for fun: color emojis for the icon

Example: `dzvol -i "^fn(Noto Color Emoji:size=16)👻"`

### Minor edits

* -h without a parm = help.
* Minor edits to the help output.
* Say something if it aborts due to the lock file.

Screenshot
----------
![dzvol screenshot](screenshot.png)

Arguments to get this look would be:

    dzvol -bg '#222222' -fg '#FFFFFF' -fn 'Deja Vu Sans Mono:size=12'

How can I use it?
-----------------
Put dzvol wherever you manage your volume keys, whether it's through
xbindkeys or your WM.

For example, in i3, I would do something like this:

    set $dzvol dzvol -bg '#222222' -fg '#FFFFFF' -fn 'Deja Vu Sans Mono 12' &
    bindsym XF86AudioRaiseVolume exec --no-startup-id amixer -qD pulse set Master 2%+ unmute && $dzvol

Notice how I make dzvol fork, since it **does not fork on its own**, but it *will*
only allow one instance of itself running, so you won't flood your screen.

If you don't fork it, you may not be able to continue adjusting volume using the same key.

Why make *another* one?
-----------------------
I use [i3wm](http://i3wm.org/) and have a 1 second refresh rate on my
status bar. Whenever I want to adjust my volume, I have to hold down my
volume keys, then wait a second or two to see what the percentage is, and
I didn't want a big interface to handle it for me, like KMix.

Searching for an existing one, I came across
[bruenig's post on the Arch Linux forums](https://bbs.archlinux.org/viewtopic.php?id=46608).
It looked good, but it didn't work how I imagined. I wanted one that can adjust in real-time
to the changing volume. I couldn't find any that existed, so I made dzvol.

Command Line Paramters
----------------------
Most command line parameters are similar to that of dzen2, and work the same way,
and most parameters are sent directly *to* dzen2 for interpretation.

There *are* some custom ones, which I'll go over here:

|argument|description|
|--------|-----------|
|-h, --help  | Show the help message and exit.|
|-x X        | Adjust the X position of the window manually. If left default (-1), it will center based on your screen dimensions.|
|-y Y        | Adjust the Y position of the window manually. If left default (-1), it will center based on your screen dimensions.|
|-w WIDTH    | Adjust the width of the window manually (default is 256). The progress bar and text placement should scale keeping preservation of proportions.|
|-h HEIGHT   |Adjust the height of the window (default is 32). Also keeps preservation of proportions.|
|-d, --delay DELAY |Sets the time it takes to exit when the volume hasn't changed. Defaults to 2 seconds.|
|-bg COLOR | Sets the background color. This should be in the same format dzen2 would accept.|
|-fg COLOR | Sets the foreground color. This should be in the same format dzen2 would accept.|
|-fn FONT  | Sets the font. This should be in the same format dzen2 would accept.|
|-i TEXT   | Sets the icon text/character on the left. Defaults to a music note seen in the screenshot. |
|-s, --speed SPEED | Speed in microseconds to poll ALSA for volume. The higher the value, the slower the polling, the lower the value, the faster the polling. A lower value would result in a smoother animation as the volume changes; higher values get choppier, but save on CPU. Values below 20,000 begin to cause high CPU usage. Defaults to 50,000.|
|-p, --pulseaudio | Get volume from Pulseaudio (default is Alsa)|
|-m, --max MAX    | Sets volume maximum (default: 100)|

