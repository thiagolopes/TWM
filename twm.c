#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

int main (int argc, char **argv){
  printf("twm starts\n");

  /* basic xcb vars */
  xcb_connection_t *con;
  xcb_screen_iterator_t screen;
  xcb_window_t window;
  unsigned short width_in_pixels, height_in_pixels;

  con = xcb_connect(NULL, NULL);
  if (xcb_connection_has_error(con)){
    errx(1, "cannot open display %s\n", getenv("DISPLAY"));
  }

  screen = xcb_setup_roots_iterator(xcb_get_setup(con));
  window = screen.data->root;
  width_in_pixels = screen.data->width_in_pixels;
  height_in_pixels = screen.data->height_in_pixels;

  /* atoms from mcwm */

  xcb_grab_key(con, 1, window, XCB_MOD_MASK_2, XCB_NO_SYMBOL,
	       XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

  xcb_grab_button(con, 0, window, XCB_EVENT_MASK_BUTTON_PRESS |
		  XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
		  XCB_GRAB_MODE_ASYNC, window, XCB_NONE, 1, XCB_MOD_MASK_1);

  xcb_grab_button(con, 0, window, XCB_EVENT_MASK_BUTTON_PRESS |
		  XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
		  XCB_GRAB_MODE_ASYNC, window, XCB_NONE, 3, XCB_MOD_MASK_1);

  xcb_flush(con);
  xcb_generic_event_t *ev;
  for (;;)
    {
      ev = xcb_wait_for_event(con);
      switch (ev->response_type & ~0x80) {
      case XCB_BUTTON_PRESS:
	printf("AAAAAAAAAAA");
      case XCB_MOTION_NOTIFY:
	printf("AAAAAAAAAAA");
      case XCB_BUTTON_RELEASE:
	printf("AAAAAAAAAAA");
      }}

  while(1) {
    xcb_generic_event_t *event = xcb_wait_for_event(con);

    switch(event->response_type & ~0x80){
    case XCB_KEY_PRESS: {
      printf("key pressed!!!\n");
      break;
    }
    case XCB_BUTTON_PRESS: {
      xcb_button_press_event_t *press = (xcb_button_press_event_t *)event;
      printf("button pressed!!!\n");
      exit(1);
    }
      free(event);
    }}

  char *xterm_args[] = {"xterm", NULL};
	execvp("xterm", xterm_args);

	/* end wm, disconect server */
	xcb_disconnect(con);
	return 0;
}
