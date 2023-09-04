#include "xutils.h"

int GetCursorXPos(Display *display)
{

    Window root = DefaultRootWindow(display);
    XEvent event;
    XQueryPointer(display, root, &event.xbutton.root, &event.xbutton.window,
                  &event.xbutton.x_root, &event.xbutton.y_root,
                  &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
    return event.xbutton.x;
}

bool getActiveWindowAndProcessName(std::string &windowTitle, std::string &processName)
{
  xcb_connection_t *connection = xcb_connect(nullptr, nullptr);
  if (xcb_connection_has_error(connection))
  {
    //std::cerr << "Failed to connect to X server." << std::endl;
    return false;
  }

  xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
  xcb_window_t root = screen->root;

  xcb_ewmh_connection_t ewmh_connection;

  xcb_intern_atom_cookie_t *atom_cookie = xcb_ewmh_init_atoms(connection, &ewmh_connection);
  if (!xcb_ewmh_init_atoms_replies(&ewmh_connection, atom_cookie, nullptr))
  {
    //std::cerr << "Failed to initialize EWMH atoms." << std::endl;
    return 1;
  }

  xcb_get_property_cookie_t cookie = xcb_ewmh_get_active_window(&ewmh_connection, 0);
  xcb_window_t window = XCB_NONE;
  xcb_generic_error_t *error = nullptr;
  if (xcb_ewmh_get_active_window_reply(&ewmh_connection, cookie, &window, &error) != 1)
  {
    //std::cerr << "Failed to get active window." << std::endl;
  }
  else
  {
    //std::cout << "Active window XID: " << window << std::endl;
  }

  xcb_icccm_get_text_property_reply_t wm_name_reply;
  if (xcb_icccm_get_wm_name_reply(
          connection, xcb_icccm_get_wm_name(connection, window), &wm_name_reply, nullptr) == 1)
  {
    //std::cout << "Active window title: " << wm_name_reply.name << std::endl;
    windowTitle.assign(wm_name_reply.name);
    xcb_icccm_get_text_property_reply_wipe(&wm_name_reply);
  }

  xcb_icccm_get_wm_class_reply_t wm_class_reply;
  if (xcb_icccm_get_wm_class_reply(
          connection, xcb_icccm_get_wm_class(connection, window), &wm_class_reply, nullptr) == 1)
  {
    //std::cout << "Active process name: " << wm_class_reply.class_name << std::endl;
    processName.assign(wm_class_reply.class_name);
    xcb_icccm_get_wm_class_reply_wipe(&wm_class_reply);
  }

  xcb_ewmh_connection_wipe(&ewmh_connection);
  xcb_disconnect(connection);
  return 0;
}