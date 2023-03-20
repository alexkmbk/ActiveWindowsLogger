// -*- coding:utf-8-unix; mode:c; -*-
//
// get the active window on X window system
//

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <iostream>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

// #include <X11/Xlib.h>           // `apt-get install libx11-dev`
// #include <X11/Xmu/WinUtil.h>    // `apt-get install libxmu-dev`

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <sys/time.h>

// Bool xerror = False;

using std::string;

bool getActiveWindowAndProcessName(string &windowTitle, string &processName)
{
  xcb_connection_t *connection = xcb_connect(nullptr, nullptr);
  if (xcb_connection_has_error(connection))
  {
    std::cerr << "Failed to connect to X server." << std::endl;
    return false;
  }

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
  xcb_window_t root = screen->root;

  xcb_ewmh_connection_t ewmh_connection;

  xcb_intern_atom_cookie_t *atom_cookie = xcb_ewmh_init_atoms(connection, &ewmh_connection);
  if (!xcb_ewmh_init_atoms_replies(&ewmh_connection, atom_cookie, nullptr))
  {
    std::cerr << "Failed to initialize EWMH atoms." << std::endl;
    return 1;
  }

  xcb_get_property_cookie_t cookie = xcb_ewmh_get_active_window(&ewmh_connection, 0);
  xcb_window_t window = XCB_NONE;
  xcb_generic_error_t *error = nullptr;
  if (xcb_ewmh_get_active_window_reply(&ewmh_connection, cookie, &window, &error) != 1)
  {
    std::cerr << "Failed to get active window." << std::endl;
  }
  else
  {
    std::cout << "Active window XID: " << window << std::endl;
  }

  xcb_icccm_get_text_property_reply_t wm_name_reply;
  if (xcb_icccm_get_wm_name_reply(
          connection, xcb_icccm_get_wm_name(connection, window), &wm_name_reply, nullptr) == 1)
  {
    std::cout << "Active window title: " << wm_name_reply.name << std::endl;
    windowTitle.assign(wm_name_reply.name);
    xcb_icccm_get_text_property_reply_wipe(&wm_name_reply);
  }

  xcb_icccm_get_wm_class_reply_t wm_class_reply;
  if (xcb_icccm_get_wm_class_reply(
          connection, xcb_icccm_get_wm_class(connection, window), &wm_class_reply, nullptr) == 1)
  {
    std::cout << "Active process name: " << wm_class_reply.class_name << std::endl;
    processName.assign(wm_class_reply.class_name);
    xcb_icccm_get_wm_class_reply_wipe(&wm_class_reply);
  }

  xcb_ewmh_connection_wipe(&ewmh_connection);
  xcb_disconnect(connection);
  return 0;
}

void handle_signal(int sig)
{
  std::cout << "Received signal: " << sig << std::endl;
  string windowTitle = "";
  string processName = "";
  getActiveWindowAndProcessName(windowTitle, processName);
}

int main(void)
{
  setlocale(LC_ALL, ""); // see man locale

  Display *display = XOpenDisplay(NULL);

  if (!display)
  {
    fprintf(stderr, "Failed to open display\n");
    return 1;
  }



  // getActiveWindowAndProcessName(windowTitle, processName);

  // Install signal handler for SIGALRM
  signal(SIGALRM, handle_signal);

  // Create a timer that will expire every 1 second
  struct itimerval timer;
  timer.it_value.tv_sec = 1;
  timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = 1;
  timer.it_interval.tv_usec = 0;

  if (setitimer(ITIMER_REAL, &timer, NULL) == -1)
  {
    std::cerr << "Failed to set timer." << std::endl;
    exit(EXIT_FAILURE);
  }

Window window;
    int screen_num = DefaultScreen(display);
    unsigned long background = WhitePixel(display, screen_num);
    unsigned long border = BlackPixel(display, screen_num);

XSetWindowAttributes attributes;
attributes.background_pixel = background;
attributes.border_pixel = border;;

    window = XCreateWindow(display, DefaultRootWindow(display), 0, 0, 400, 300, 0,
                           DefaultDepth(display, screen_num), InputOutput,
                           DefaultVisual(display, screen_num),
                           CWBackPixel | CWBorderPixel,
                           &attributes);

    //XMapWindow(display, window);
    //XFlush(display);
    // sleep(10);
    // XCloseDisplay(display);


  XScreenSaverInfo *info = XScreenSaverAllocInfo();


  // Wait for signals to arrive
  while (true)
  {

    XEvent event;
    XNextEvent(display, &event);

    switch (event.type)
    {
    case ScreenSaverActive:
      std::cout << "ScreenSaverActive event" << std::endl;
      break;
    case ScreenSaverNotify:
      std::cout << "ScreenSaverNotify" << std::endl;
      // if (event.xscreensaver.state == ScreenSaverOn) {
      //     std::cout << "Screen saver is active." << std::endl;
      // }
      // else if (event.xscreensaver.state == ScreenSaverOff) {
      //     std::cout << "Screen saver is inactive." << std::endl;
      // }
      // else if (event.xscreensaver.state == ScreenSaverCycle) {
      //     std::cout << "Screen saver is cycling." << std::endl;
      // }
      // else if (event.xscreensaver.state == ScreenSaverBlanked) {
      //     std::cout << "Screen saver is blanked." << std::endl;
      // }
      // else {
      //     // X server is locked
      //     std::cout << "X server is locked." << std::endl;
      // }
      break;
    default:
      break;
    }

    // XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);

    // if (info->state == ScreenSaverOn)
    // {
    //   printf("Screen saver is active\n");
    // }
    // else if (info->state == ScreenSaverOff)
    // {
    //   printf("Screen saver is inactive\n");
    // }
    // else if (info->state == ScreenSaverBlanked)
    // {
    //   printf("Screen is locked\n");
    //   // Do something when screen is locked
    // }

    sleep(1);
  }

  XFree(info);
  XCloseDisplay(display);

  return 0;
}