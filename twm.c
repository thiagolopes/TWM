#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#define XCB_EVENT_BIT_MASK ~0x80

int new_process(char *programm);

xcb_connection_t *con;
xcb_screen_iterator_t screen;
xcb_window_t window;
unsigned short width_in_pixels, height_in_pixels;
xcb_generic_event_t *ev;


int main (int argc, char **argv) {
    printf("hello, twm starts\n");

    con = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(con)) {
        errx(1, "cannot open display %s\n", getenv("DISPLAY"));
    }

    screen = xcb_setup_roots_iterator(xcb_get_setup(con));
    window = screen.data->root;
    width_in_pixels = screen.data->width_in_pixels;
    height_in_pixels = screen.data->height_in_pixels;

    /* TODO add atoms */

    /* xcb_grab_key grab keys from window manager */
    xcb_grab_key(con, 1, window, XCB_MOD_MASK_ANY, XCB_GRAB_ANY,
                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

    /* xcb_grab_button grab button from a window  */
    xcb_grab_button(con, 0, window, XCB_EVENT_MASK_BUTTON_PRESS |
                    XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC,
                    XCB_GRAB_MODE_ASYNC, window, XCB_NONE, 1, XCB_MOD_MASK_1);

    /* sync to applay */
    xcb_flush(con);

    while((ev = xcb_wait_for_event(con))) {
        switch (ev->response_type & XCB_EVENT_BIT_MASK) {
        case XCB_KEY_RELEASE: {
            xcb_key_press_event_t *kev = (xcb_key_press_event_t*) ev;

            if (kev->state & XCB_MOD_MASK_CONTROL) {
                printf("CTRL was released\n");
            }
            else if (kev->state & XCB_MOD_MASK_4) {
                new_process("xterm");
            }
            else {
                printf("Key key was released, but don't know which one\n");
            }
        }
        }
    }

    /* end wm, disconect server */
    xcb_disconnect(con);
    printf("byebye!\n");
    return 0;
}

int new_process(char* programm) {
    /* create a new process based in programm name in PATH */
    pid_t pid, sid;
    pid = fork();

    if (pid == -1) {
        /* error */
        errx(1, "error to get fork: %s, pid: %d", programm, getpid());
    } else if (pid == 0) {
        /* child process */
        sid = setsid();
        if (sid == -1) {
            errx(1, "error to set sid, pid: %d", pid);
        }

        if(execlp(programm, programm, NULL) == -1) {
            errx(1, "error to exec new program");
        }
	_exit(0);
    }
    return 0;
}
