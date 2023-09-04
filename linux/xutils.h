#ifndef INCLUDED_xutils_h
#define INCLUDED_xutils_h

#include <string>

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

// #include <X11/Xlib.h>           // `apt-get install libx11-dev`
// #include <X11/Xmu/WinUtil.h>    // `apt-get install libxmu-dev`

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

int GetCursorXPos(Display *display);
bool getActiveWindowAndProcessName(std::string &windowTitle, std::string &processName);

#endif