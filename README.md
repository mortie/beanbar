# Beanbar

Beanbar is a status bar application based on web technologies running WebKitGTK.

![](https://raw.githubusercontent.com/mortie/beanbar/master/img/screenshot.png)

## A web browser in my status bar? Really?

Well, this status bar probably isn't for people who's extremely concerned about
bloat, and Beanbar does use more memory than most other status bars. However,
using web technologies means this bar is extremely configurable. Any part of
the design can be designed through a few lines of CSS, not just predefined
configurable colors or sizes, and writing custom modules becomes really easy
when it's just a couple of lines of javascript.

## Known bugs

* Modules like i3workspaces and audio should show information with the same
  order/color across multiple bars on multiple monitors.
* Occasionally, especially when the CPU is under heavy load, the bar doesn't
  position itself correctly. Why?
