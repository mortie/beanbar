# Beanbar

Beanbar is a status bar application based on web technologies running WebKitGTK.

![](https://raw.githubusercontent.com/mortie/beanbar/master/img/screenshot.png)

<!-- toc -->

- [Installation](#installation)
- [Goals](#goals)
- [A web browser in my status bar? Really?](#a-web-browser-in-my-status-bar-really)
- [Configuration](#configuration)
  * [init(...modules)](#initmodules)
  * [config(obj)](#configobj)
  * [css(string)](#cssstring)
  * [await include(src)](#await-includesrc)
- [Known bugs/issues](#known-bugsissues)

<!-- tocstop -->

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

Well, Benabar probably isn't for people who's extremely concerned about bloat,
and it does use more memory than most other status bars. However,
using web technologies is probably the easiest and arguably best way to achieve
the hackability goal.

I haven't used a computer with less than 16GB of RAM for quite a while, so I'm
not exactly starved for RAM.

## Configuration

The config file defaults to `$XDG_CONFIG_HOME/beanbar/beanbar.js`
(or `~/.config/beanbar/beanbar.js`). For an example config file, see
[example-conf.js](https://github.com/mortie/beanbar/blob/master/example-conf.js).

The directory which the config file is in cts as a web root,
where additional assets can be loaded from.

The current list of built-in modules is:

* `h(Label, { text })`: Just arbitrary text.
* `h(Launcher, { cmd, text })`: A launcher, which runs `cmd` when clicked.
* `h(Drawer, { timeout }, ...)`: A drawer which expands to display its children
  when clicked.
* `h(Audio)`: Show the currently connected audio devices.
* `h(Disk, { mount })`: Show the free space in `mount`.
* `h(Battery, { low })`: Show the battery level.
* `h(Network)`: Show the connected network connections.
* `h(Memory)`: Show the percentage of free memory.
* `h(Processor)`: Show the CPU usage.
* `h(Time, { func })`: Show the current time.
* `h(I3Workspaces, { scroll })`: Show workspaces from i3wm. If `scroll` is
  true, scrolling on the bar will change workspace.

The main functions you will use are:

### init(...modules)

This function should generally come last, as it depends on state set by the
config and css functions.

`init();` is the most important function; it's where you configure what modules
to use. Its arguments are the return value of the `h` function, which you can
read about in
[Preact's documentation](https://preactjs.com/guide/api-reference).

### config(obj)

The `config` function sets general config options. Currently, the only option
is `updateTime`, which is the number of milliseconds between updating polling
modules such as the disk and battery modules.

### css(string)

The main use cases are:

* Applying a pre-defined theme. Currently, the only built-in alternate theme is in
  the global variable called `THEME_DARK`, so `css(THEME_DARK);` will enable a
  dark theme.
* Overwriting CSS variables. Look at
  [style.css](https://github.com/mortie/beanbar/blob/master/web/style.css) to
  see which variables can be overwritten.
* Just generally hacking the look of the bar. If you're doing this, here's a
  couple of things you should know:
	* Use [multi-line template literals](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals)
	  for a less annoying syntax.
	* The web inspector should come in useful (right click on the bar, click "inspect
	  element").
	* A module can be targeted with the `module.<module-name>` selector, and a widget
	  with `widget.<widget-name>`.

### await include(src)

Include another javascript source file. The `include` function returns a
promise, but the main config file is executed in an async function context, it
can `await` the promise from the top level.

## Known bugs/issues

* Modules like i3workspaces and audio should show information with the same
  order/color across multiple bars on multiple monitors.
* Occasionally, especially when the CPU is under heavy load, the bar doesn't
  position itself correctly. Why?
* Some options such as the bar's height/location and what monitor it appears on,
  are needed before any javascript is loaded, and can therefore only be
  provided on the command line, not through the config javascript file.
