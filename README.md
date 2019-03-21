# Beanbar

Beanbar is a status bar application based on web technologies running WebKitGTK.

![](https://raw.githubusercontent.com/mortie/beanbar/master/img/screenshot.png)

## Installation

To build Beanbar and install it to /usr/local/bin,
just run `make && sudo make install`.

Dependencies:

* GTK 3 (on X11; I haven't looked into making Beanbar work on Wayland yet)
* WebKitGTK
* libdbus
* libpulse

## Goals

* I want a status bar which reacts instantly to changes in my system. The
  instant NetworkManager starts trying to connect to a network, it should show
  up with a loading icon, and that loading icon should disappear the instant
  the network is actually connected. The second a new audio device is
  connected, a new volume indicator for that device should show up, and changes
  in volume should be reflected the moment they happen. Whenever a new monitor
  is connected, a new bar window should immediately show up on that monitor,
  and that window should immdiately go away when the monitor goes away.
* I want a hackable status bar, where changing anything is just a small config
  change, where a custom module is a few lines of javascript and changing the
  existing look of a module completely is just a few lines of CSS, without
  having to recompile the bar.
* I want an interactive bar, which doesn't just reflect the state of the
  system, but also makes it possible to change the system, launch applications,
  etc. This is especially nice because my laptop is a 2-in-1 laptop with a
  touch screen, and even though I'm using i3wm, I want to be able to do things
  when in laptop mode.

Beanbar currently does a pretty good job of the first two points, but it's not
quite as interactive as I would want it to be yet.

## A web browser in my status bar? Really?

Well, this status bar probably isn't for people who's extremely concerned about
bloat, and Beanbar does use more memory than most other status bars. However,
using web technologies is probably the easiest and arguably best way to achieve
the hackability goal.

I haven't used a computer with less than 16GB of RAM for quite a while, so I'm
not exactl starved for RAM, and WebKitGTK doesn't seem to be _too_ bad when it
comes to CPU usage.

## Known bugs

* Modules like i3workspaces and audio should show information with the same
  order/color across multiple bars on multiple monitors.
* Occasionally, especially when the CPU is under heavy load, the bar doesn't
  position itself correctly. Why?
