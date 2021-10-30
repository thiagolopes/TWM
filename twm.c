#include <X11/keysym.h>
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#define TERMINAL "st"
#define APPLICATIONS_MENU "dmenu_run"

#define META_MASK XCB_MOD_MASK_4
#define ALT_MASK XCB_MOD_MASK_1
#define SHIFT_MASK XCB_MOD_MASK_SHIFT
#define CONTROL XCB_MOD_MASK_CONTROL

int run;
unsigned short width_in_pixels, height_in_pixels;
xcb_connection_t *con;
xcb_screen_iterator_t screen;
xcb_window_t window;
xcb_generic_event_t *ev;
xcb_key_symbols_t *keysyms;
uint32_t masks[] = {
    /* these only one window can have (the window manager).
       if a error occurs to set, other window manager are running.*/
    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE};

int new_process(char *programm);
void key_press_process(xcb_key_press_event_t *ev);
void map_request_process(xcb_map_request_event_t *mrev);

int main(int argc, char **argv) {
  printf("hello, twm starts\n");

  con = xcb_connect(NULL, NULL);
  if (xcb_connection_has_error(con)) {
    errx(1, "cannot open display %s\n", getenv("DISPLAY"));
  }

  screen = xcb_setup_roots_iterator(xcb_get_setup(con));
  window = screen.data->root;
  width_in_pixels = screen.data->width_in_pixels;
  height_in_pixels = screen.data->height_in_pixels;
  keysyms = xcb_key_symbols_alloc(con);

  // TODO add atoms

  /* :XCB_CW_EVENT_MASK:
     The event-mask defines which events the
     client is interested in for this window
     (or for some event types, inferiors of the window). */
  xcb_change_window_attributes(con, window, XCB_CW_EVENT_MASK, masks);

  // remove all key events
  xcb_ungrab_key(con, XCB_GRAB_ANY, window, XCB_MOD_MASK_ANY);

  /* subscribe new keys.
     all keycodes needed to subscribe */
  xcb_keycode_t *XK_Return_KC = xcb_key_symbols_get_keycode(keysyms, XK_Return);
  xcb_keycode_t *XK_d_KC = xcb_key_symbols_get_keycode(keysyms, XK_d);
  xcb_keycode_t *XK_q_KC = xcb_key_symbols_get_keycode(keysyms, XK_q);

  // TODO refactor to generate from a config
  xcb_grab_key(con, 1, window, META_MASK, *XK_Return_KC, // meta+Return
               XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_grab_key(con, 1, window, META_MASK, *XK_d_KC, // meta+d
               XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
  xcb_grab_key(con, 1, window, META_MASK | SHIFT_MASK, *XK_q_KC, // meta+shift+q
               XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

  // sync to applay
  xcb_flush(con);

  run = 1;
  while (run) {
    ev = xcb_wait_for_event(con);
    printf("event: %s\n", xcb_event_get_label(ev->response_type));

    switch (XCB_EVENT_RESPONSE_TYPE(ev)) {
    case XCB_KEY_PRESS: {
      xcb_key_press_event_t *kev = (xcb_key_press_event_t *)ev;
      key_press_process(kev);
    }

    case XCB_MAP_REQUEST: {
      xcb_map_request_event_t *mrev = (xcb_map_request_event_t *)ev;
      map_request_process(mrev);
    }
    }
  }

  // end wm, disconect server
  xcb_key_symbols_free(keysyms);
  xcb_disconnect(con);
  printf("byebye!\n");
  exit(0);
}

void map_request_process(xcb_map_request_event_t *mrev) {
  // masks to enable focus and mouse focus to window
  uint32_t values[] = {XCB_EVENT_MASK_ENTER_WINDOW |
                       XCB_EVENT_MASK_FOCUS_CHANGE};

  xcb_map_window(con, mrev->window);
  xcb_change_window_attributes(con, mrev->window, XCB_CW_EVENT_MASK, values);
  xcb_flush(con);
}

void key_press_process(xcb_key_press_event_t *kev) {
  /*
    key->detail = key pressed
    key->state = Mod combination
   */
  xcb_keycode_t keycode = kev->detail;
  xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0);

  switch (kev->state) {
  // TODO refactor to generate from a config (2)
  case META_MASK:
    switch (keysym) {
    case XK_Return:
      new_process(TERMINAL);
      break;
    case XK_d:
      new_process(APPLICATIONS_MENU);
      break;
    }
    break;

  case META_MASK | SHIFT_MASK:
    switch (keysym) {
    case XK_q:
      run = 0;
    }
  }
}

int new_process(char *programm) {
  // create a new process based in programm name in PATH
  pid_t pid, sid;
  pid = fork();

  if (pid == -1) {
    // error
    errx(1, "error to get fork: %s, pid: %d", programm, getpid());
  } else if (pid == 0) {
    // child process
    sid = setsid();
    if (sid == -1) {
      errx(1, "error to set sid, pid: %d", pid);
    }

    if (execlp(programm, programm, NULL) == -1) {
      errx(1, "error to exec new program");
    }
    _exit(0);
    wait(NULL);
  }
  return 0;
}
