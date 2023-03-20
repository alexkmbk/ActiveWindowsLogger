// -*- coding:utf-8-unix; mode:c; -*-
//
// get the active window on X window system
//

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <iostream>
#include <string.h>

#include <X11/Xlib.h>           // `apt-get install libx11-dev`
#include <X11/Xmu/WinUtil.h>    // `apt-get install libxmu-dev`


#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

Bool xerror = False;

bool getProcessName(Window w) {
xcb_connection_t *connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(connection)) {
        std::cerr << "Failed to connect to X server." << std::endl;
        return false;
    }

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    xcb_window_t root = screen->root;

    xcb_ewmh_connection_t ewmh_connection;
    
    xcb_intern_atom_cookie_t* atom_cookie = xcb_ewmh_init_atoms(connection, &ewmh_connection);
    if (!xcb_ewmh_init_atoms_replies(&ewmh_connection, atom_cookie, nullptr)) {
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

//  xcb_ewmh_get_utf8_strings_reply_t ewmh_txt_prop;
//  xcb_icccm_get_text_property_reply_t icccm_txt_prop;
//  ewmh_txt_prop.strings = icccm_txt_prop.name = NULL;

// if (window != XCB_NONE && (xcb_ewmh_get_wm_name_reply(&ewmh_connection, xcb_ewmh_get_wm_name(&ewmh_connection, window), &ewmh_txt_prop, NULL) == 1 || xcb_icccm_get_wm_name_reply(connection, xcb_icccm_get_wm_name(connection, window), &icccm_txt_prop, NULL) == 1)) {
//         char *src = NULL;
//         size_t title_len = 0;
//         if (ewmh_txt_prop.strings != NULL) {
//             src = ewmh_txt_prop.strings;
//             title_len = ewmh_txt_prop.strings_len;
//         } else if (icccm_txt_prop.name != NULL) {
//             src = icccm_txt_prop.name;
//             title_len = icccm_txt_prop.name_len;
//         }
//         if (src != NULL) {
//             //strncpy(title, src, title_len);
//             //title[title_len] = '\0';
//         std::cout << "Active window title: " << src << std::endl;
//         }
//     }

xcb_icccm_get_text_property_reply_t wm_name_reply;
    if (xcb_icccm_get_wm_name_reply(
        connection, xcb_icccm_get_wm_name(connection, window), &wm_name_reply, nullptr) == 1) {
        std::cout << "Active window title: " << wm_name_reply.name << std::endl;
        xcb_icccm_get_text_property_reply_wipe(&wm_name_reply);
    }

  /*cookie = xcb_ewmh_get_wm_name(&ewmh_connection, window);
    xcb_get_property_reply_t* property_reply  = xcb_get_property_reply(connection, cookie, nullptr);

    if (property_reply != nullptr) {
        std::cout << "Active window title: " << xcb_get_property_value(property_reply)
                  << std::endl;
        free(property_reply);
    }*/


    // cookie = xcb_icccm_get_wm_class(connection, window);
    // xcb_icccm_get_wm_class_reply_t reply;
    // error = nullptr;
    // if (xcb_icccm_get_wm_class_reply(connection, cookie, &reply, &error) == 1) {
    //     std::cout << "Process name: " << reply.class_name << std::endl;
    //     free(reply.class_name);
    // } else {
    //     std::cerr << "Failed to get process name." << std::endl;
    // }

xcb_icccm_get_wm_class_reply_t wm_class_reply;
    if (xcb_icccm_get_wm_class_reply(
        connection, xcb_icccm_get_wm_class(connection, window), &wm_class_reply, nullptr) == 1) {
        std::cout << "Active process name: " << wm_class_reply.class_name << std::endl;
        xcb_icccm_get_wm_class_reply_wipe(&wm_class_reply);
    }


    xcb_ewmh_connection_wipe(&ewmh_connection);
    xcb_disconnect(connection);
    return 0;
}

Display* open_display(){
  printf("connecting X server ... ");
  Display* d = XOpenDisplay(NULL);
  if(d == NULL){
    printf("fail\n");
    exit(1);
  }else{
    printf("success\n");
  }
  return d;
}

int handle_error(Display* display, XErrorEvent* error){
  printf("ERROR: X11 error\n");
  xerror = True;
  return 1;
}

Window get_focus_window(Display* d){
  Window w;
  int revert_to;
  printf("getting input focus window ... ");
  XGetInputFocus(d, &w, &revert_to); // see man
  if(xerror){
    printf("fail\n");
    exit(1);
  }else if(w == None){
    printf("no focus window\n");
    exit(1);
  }else{
    printf("success (window: %d)\n", (int)w);
  }

  return w;
}

// get the top window.
// a top window have the following specifications.
//  * the start window is contained the descendent windows.
//  * the parent window is the root window.
Window get_top_window(Display* d, Window start){
  Window w = start;
  Window parent = start;
  Window root = None;
  Window *children;
  unsigned int nchildren;
  Status s;

  printf("getting top window ... \n");
  while (parent != root) {
    w = parent;
    s = XQueryTree(d, w, &root, &parent, &children, &nchildren); // see man

    if (s)
      XFree(children);

    if(xerror){
      printf("fail\n");
      exit(1);
    }

    printf("  get parent (window: %d)\n", (int)w);
  }

  printf("success (window: %d)\n", (int)w);

  return w;
}

// search a named window (that has a WM_STATE prop)
// on the descendent windows of the argment Window.
Window get_named_window(Display* d, Window start){
  Window w;
  printf("getting named window ... ");
  w = XmuClientWindow(d, start); // see man
  if(w == start)
    printf("fail\n");
  printf("success (window: %d)\n", (int) w);
  return w;
}

// (XFetchName cannot get a name with multi-byte chars)
void print_window_name(Display* d, Window w){
  XTextProperty prop;
  Status s;

  printf("window name:\n");

  s = XGetWMName(d, w, &prop); // see man
  if(!xerror && s){
    int count = 0, result;
    char **list = NULL;
    result = XmbTextPropertyToTextList(d, &prop, &list, &count); // see man
    if(result == Success){
      printf("\t%s\n", list[0]);
    }else{
      printf("ERROR: XmbTextPropertyToTextList\n");
    }
  }else{
    printf("ERROR: XGetWMName\n");
  }
}

/*void print_window_class(Display* d, Window w){
  Status s;
  XClassHint* class;

  printf("application: \n");

  class = XAllocClassHint(); // see man
  if(xerror){
    printf("ERROR: XAllocClassHint\n");
  }

  s = XGetClassHint(d, w, class); // see man
  if(xerror || s){
    printf("\tname: %s\n\tclass: %s\n", class->res_name, class->res_class);
  }else{
    printf("ERROR: XGetClassHint\n");
  }
}*/

void print_window_info(Display* d, Window w){
  printf("--\n");
  print_window_name(d, w);
  //print_window_class(d, w);
}

int main(void){
  Display* d;
  Window w;

  // for XmbTextPropertyToTextList
  setlocale(LC_ALL, ""); // see man locale

  d = open_display();
  XSetErrorHandler(handle_error);

  // get active window
  w = get_focus_window(d);
  w = get_top_window(d, w);
  w = get_named_window(d, w);

  print_window_info(d, w);
  getProcessName(w);
}