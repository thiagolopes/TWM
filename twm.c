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
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "twm.h"

int main(int argc, char **argv)
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

	/*
	 * !TODO Add atoms here
	 */

	/*
	 * XCB_CW_EVENT_MASK
	 *    The event-mask defines which events the
	 *    client is interested in for this window
	 *    (or for some event types, inferiors of the window).
	 */
	xcb_change_window_attributes(con, window,
				     XCB_CW_EVENT_MASK, window_masks);

	/*
	 * Remove all key events to ensure
	 */
	xcb_ungrab_key(con, XCB_GRAB_ANY, window, XCB_MOD_MASK_ANY);

	/*
	 * !TODO maybe move these to a configuration
	 */
	struct Keybind keybinds[] = {
		{
			/*
			 * Meta+Return
			 */
			META_MASK,
			xcb_key_symbols_get_keycode(keysyms, XK_Return)
		},

		{
			/*
			 * Meta+d
			 */
			META_MASK,
			xcb_key_symbols_get_keycode(keysyms, XK_d)
		},

		{
			/*
			 * Meta+Shift+q
			 */
			META_MASK | SHIFT_MASK,
			xcb_key_symbols_get_keycode(keysyms, XK_q)
		},
	};
	struct ButtonAction button_actions[] = {
		{
			ALT_MASK,
			XCB_BUTTON_MASK_1,
		},{
			ALT_MASK,
			XCB_BUTTON_MASK_3,
		},
	};

        /*
	* Subscribe new keys/buttons.
        * All keycodes needed to subscribe
	*/
	for (int k = 0; k < LEN(keybinds); ++k) {
		xcb_grab_key(con, 1, window,
			     keybinds[k].modifiers, *keybinds[k].key,
			     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
	}
	for (int b = 0; b <LEN(button_actions); ++b) {
		struct ButtonAction *button = &button_actions[b];
		xcb_grab_button(
			con, 1, window,
			(XCB_EVENT_MASK_BUTTON_1_MOTION |
			 XCB_EVENT_MASK_BUTTON_3_MOTION),
			XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, window,
			XCB_NONE, button->cursor, button->modfiers);
	}

	/* Sync all buffers */
	xcb_flush(con);

	run = 1;
	while (run) {
		ev = xcb_wait_for_event(con);
		if (ev->response_type == 0) {
			xcb_generic_error_t *error = (xcb_generic_error_t *) ev;
			printf("event-error-code: %d, "
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

void motion_notify_handler(xcb_motion_notify_event_t *mnev)
{
        /*
	 * This is not really a real notify, but just a hint that
	 * the mouse pointer moved. This means we need to get the
	 * current pointer position ourselves.
	 */
	xcb_query_pointer_reply_t *pointer;
	pointer = query_pointer(mnev->root);

	/*
	 * !TODO Refactor to generate from a config (2)
	 */

 	switch (mnev->state) {
	case (ALT_MASK | XCB_EVENT_MASK_BUTTON_1_MOTION):
		printf("moving...\n");
		break;
	case (ALT_MASK | XCB_EVENT_MASK_BUTTON_3_MOTION):
		printf("resizing...\n");
		break;
	}

	free(pointer);
}

xcb_query_pointer_reply_t *query_pointer(xcb_drawable_t window)
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

xcb_get_geometry_reply_t *get_geometry(xcb_drawable_t window)
{
	xcb_get_geometry_cookie_t cookie =
		xcb_get_geometry(con, window);
	xcb_get_geometry_reply_t *geometry =
		xcb_get_geometry_reply(con, cookie, NULL);

	if (geometry == NULL) {
		errx(1, "could get a window geoemtry");
	}
	return geometry;
}


void map_request_handler(xcb_map_request_event_t *mrev)
{
	/*
	 * When a window as to ve mapped (draw) to screen it does:
	 *     The window is mapped.
	 *     Some window configs is apply: position and border width
	 *     Some attributes is apply: event and border color attributes
	 */


	xcb_get_geometry_reply_t *geometry = get_geometry(mrev->window);
	xcb_event_mask_t events_masks[] = {
		XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE
	};
	xcb_config_window_t window_configs_masks[] = {
		XCB_CONFIG_WINDOW_X |
		XCB_CONFIG_WINDOW_Y |
		XCB_CONFIG_WINDOW_BORDER_WIDTH
	};

	/*
	 * Initially all the windows is mapped on center screen. *\/
	 * !TODO Implement some cascade windows position*\/
         * !IDEIA Remember the last time size before close, between sessions
	 */
	uint32_t window_configs_values[] = {
		(window_width / 2) - (geometry->width / 2),
		(window_height / 2) - (geometry->height / 2),
		BORDER_PIXEL
	};
	int border_color[] = {BORDER_COLOR};

	xcb_map_window(con, mrev->window);
	xcb_configure_window(con, mrev->window,
			     *window_configs_masks, window_configs_values);
	xcb_change_window_attributes(con, mrev->window,
				     XCB_CW_BORDER_PIXEL, border_color);
	xcb_change_window_attributes(con, mrev->window,
				     XCB_CW_EVENT_MASK, events_masks);
	/*
	 * Sync all buffers
	 */
	xcb_flush(con);
	free(geometry);
}

void key_press_handler(xcb_key_press_event_t *kev)
{
	/*
	 * @detail = Key pressed
	 * @state = Mod combination
	 */
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

int new_process(char *programm)
{
	/*
	 * Create a new process based in programm name in PATH
	 */
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
