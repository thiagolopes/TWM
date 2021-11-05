/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <X11/keysym.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "twm.h"

/*
 * !TODO move to a singleton struct
 */
int run;
unsigned short window_width, window_height;
xcb_connection_t *con;
xcb_screen_iterator_t screen;
xcb_window_t window;
xcb_generic_event_t *ev;
xcb_key_symbols_t *keysyms;

Pointer pointer_history = {0, 0};
Cord center_screen;


int
main(int argc, char **argv)
{
	printf("hello, twm starts\n");

	con = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(con)) {
		errx(1, "cannot open display %s\n", getenv("DISPLAY"));
	}

	screen = xcb_setup_roots_iterator(xcb_get_setup(con));
	window = screen.data->root;
	window_width = screen.data->width_in_pixels;
	window_height = screen.data->height_in_pixels;
	keysyms = xcb_key_symbols_alloc(con);

	center_screen.x = window_width / 2;
	center_screen.y = window_height / 2;

	/*
	 * !TODO Add atoms here
	 */

	/*
	 * XCB_CW_EVENT_MASK
	 *    The event-mask defines which events the
	 *    client is interested in for this window
	 *    (or for some event types, inferiors of the window).
	 */
	xcb_event_mask_t window_masks[] = {
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE
	};
	xcb_change_window_attributes(con, window,
				     XCB_CW_EVENT_MASK, window_masks);

	/*
	 * Remove all key events to ensure
	 */
	xcb_ungrab_key(con, XCB_GRAB_ANY, window, XCB_MOD_MASK_ANY);

	/*
	 * !TODO maybe move these to a configuration
	 */
	Keybind keybinds[] = {
		{META_MASK, XK_Return},
		{META_MASK, XK_d},
		{META_MASK | SHIFT_MASK, XK_q},
	};
	ButtonAction button_actions[] = {
		{ALT_MASK, XCB_BUTTON_MASK_1},
		{ALT_MASK, XCB_BUTTON_MASK_3},
	};

	/*
	* Subscribe new keys/buttons.
	* All keycodes needed to subscribe
	*/
	for (int k = 0; k < LEN(keybinds); ++k) {
		xcb_keycode_t *keycode = xcb_key_symbols_get_keycode(keysyms, keybinds[k].key);
		xcb_grab_key(con, 1, window,
			     keybinds[k].modifiers, *keycode,
			     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
	}
	for (int b = 0; b <LEN(button_actions); ++b) {
		ButtonAction *button = &button_actions[b];
		xcb_grab_button(
			con, 1, window,
			(XCB_EVENT_MASK_BUTTON_1_MOTION |
			 XCB_EVENT_MASK_BUTTON_3_MOTION),
			XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, window,
			XCB_NONE, button->cursor, button->modfiers);
	}

	xcb_grab_pointer(con, 1, window,
			 XCB_EVENT_MASK_POINTER_MOTION,
			 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
			 XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);

	/* Sync all buffers */
	xcb_flush(con);


	/*
	 * !TODO the wm set mouse to the middle of the screen;
	 * and add this to first pointer_history
	 */
	xcb_query_pointer_reply_t *pointer = query_pointer(window);
	xcb_warp_pointer(con, XCB_NONE, window, 0, 0, 0 ,0, center_screen.x, center_screen.y);
	pointer_history.x = center_screen.x;
	pointer_history.y = center_screen.y;
	free(pointer);

	run = 1;
	while (run) {
		ev = xcb_wait_for_event(con);
		if (ev->response_type == 0) {
			xcb_generic_error_t *error = (xcb_generic_error_t *) ev;
			printf("event_error: %d, "
			       "minor_code: %d, major_mode %d\n",
			       error->error_code,
			       error->minor_code, error->major_code);
		}
		else {
			printf("event: %s\n",
			       xcb_event_get_label(ev->response_type));
		}

		switch (XCB_EVENT_RESPONSE_TYPE(ev)) {
		case XCB_KEY_PRESS: {
			xcb_key_press_event_t *kev =
				(xcb_key_press_event_t *) ev;
			key_press_handler(kev);
			break;
		}
		case XCB_MAP_REQUEST: {
			xcb_map_request_event_t *mrev =
				(xcb_map_request_event_t *) ev;
			map_request_handler(mrev);
			break;
		}
		case XCB_MOTION_NOTIFY: {
			xcb_motion_notify_event_t *mnev =
				(xcb_motion_notify_event_t *) ev;
			motion_notify_handler(mnev);
		}}
	}

	/*
	 * End wm, disconect server
	 */
	xcb_key_symbols_free(keysyms);
	xcb_disconnect(con);
	printf("byebye!\n");
	exit(0);
}


void
motion_notify_handler(xcb_motion_notify_event_t *mnev)
{
	unsigned int values[2];
	Pointer pointer = {mnev->event_x, mnev->event_y};
	xcb_get_geometry_reply_t *geometry = get_geometry(mnev->child, false);

	/*
	 * no window grab
	 */
	if (geometry == NULL) {
		goto exit;
	}


	switch (mnev->state) {
	case (ALT_MASK | XCB_EVENT_MASK_BUTTON_1_MOTION):
		values[0] = geometry->x +
			(pointer.x - pointer_history.x);
		values[1] = geometry->y +
			(pointer.y - pointer_history.y);

		xcb_configure_window(con, mnev->child,
				     XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
				     values);
		break;
	case (ALT_MASK | XCB_EVENT_MASK_BUTTON_3_MOTION):
		values[0] = geometry->width +
			(pointer.x - pointer_history.x);
		values[1] = geometry->height +
			(pointer.y - pointer_history.y);

		xcb_configure_window(con, mnev->child,
				     XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
				     values);
		break;
	}

	exit:
	pointer_history.x = pointer.x;
	pointer_history.y = pointer.y;

	xcb_flush(con);
	free(geometry);
}


xcb_query_pointer_reply_t
*query_pointer(xcb_drawable_t window)
{
	xcb_query_pointer_cookie_t query_pointer =
		xcb_query_pointer(con, window);
	xcb_query_pointer_reply_t *pointer =
		xcb_query_pointer_reply(con, query_pointer, NULL);

	if (pointer == NULL) {
		errx(1, "could get a pointer position");
	}
	return pointer;
}


xcb_get_geometry_reply_t
*get_geometry(xcb_drawable_t window, bool exit)
{
	xcb_get_geometry_cookie_t cookie =
		xcb_get_geometry(con, window);
	xcb_get_geometry_reply_t *geometry =
		xcb_get_geometry_reply(con, cookie, NULL);

	if (geometry == NULL && exit) {
		errx(1, "could get a window geoemtry");
	}
	return geometry;
}


/*
 * When a window as to ve mapped (draw) to screen it does:
 *     The window is mapped.
 *     Some window configs is apply: position and border width
 *     Some attributes is apply: event and border color attributes
 */
void
map_request_handler(xcb_map_request_event_t *mrev)
{
	setup_window(&mrev->window);
	xcb_map_window(con, mrev->window);
	/*
	 * Sync all buffers
	 */
	xcb_flush(con);
}


/*
 * @detail = Key pressed
 * @state = Mod combination
 */
void
key_press_handler(xcb_key_press_event_t *kev)
{
	xcb_keycode_t keycode = kev->detail;
	xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0);

	/*
	 * !TODO Refactor to generate from a config (2)
	 */
	switch (kev->state) {
	case META_MASK:
		switch (keysym) {
		case XK_Return:
			new_process(TERMINAL);
			break;
		case XK_d:
			new_process(APPLICATIONS_MENU);
			break;
		}
	case META_MASK | SHIFT_MASK:
		switch (keysym) {
		case XK_q:
			run = 0;
		}
	}
}


/*
 * Create a new process based in programm name in PATH
 */
int
new_process(char *programm)
{
	pid_t pid, sid;
	pid = fork();

	if (pid == -1) {
		errx(1, "error to get fork: %s, pid: %d", programm, getpid());
	} else if (pid == 0) {
		/*
		 * Child process
		 */
		sid = setsid();
		if (sid == -1) {
			errx(1, "error to set sid, pid: %d", pid);
		}

		if (execlp(programm, programm, NULL) == -1) {
			errx(1, "error to exec new program");
		}
		_exit(0);
	}
	return 0;
}


xcb_window_t
*setup_window(xcb_window_t *window) {
	xcb_get_geometry_reply_t *geometry = get_geometry(*window, true);

	xcb_event_mask_t events_masks[] = {
		XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE
	};
	xcb_config_window_t window_configs_masks[] = {
		XCB_CONFIG_WINDOW_X |
		XCB_CONFIG_WINDOW_Y |
		XCB_CONFIG_WINDOW_BORDER_WIDTH
	};

	/*
	 * !TODO Implement some cascade windows position*\/
	 * !IDEIA Remember the last time size before close, between sessions
	 */
	uint32_t window_configs_values[] = {
		center_screen.x - (geometry->width / 2),
		center_screen.y - (geometry->height / 2),
		BORDER_PIXEL
	};
	int border_color[] = {BORDER_COLOR};

	xcb_event_mask_t checked_attributes[] = {XCB_EVENT_MASK_ENTER_WINDOW};


	xcb_configure_window(con, *window,
			     *window_configs_masks, window_configs_values);

	xcb_change_window_attributes(con, *window,
				     XCB_CW_BORDER_PIXEL, border_color);
	xcb_change_window_attributes(con, *window,
				     XCB_CW_EVENT_MASK, events_masks);

	xcb_change_window_attributes_checked(con, *window, XCB_CW_EVENT_MASK,
					     checked_attributes);

	free(geometry);
	return window;
}
