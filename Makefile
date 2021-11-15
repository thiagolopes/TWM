CFLAGS?=-Wall -g -O0 -std=c99 -pedantic
LIBS?=-lX11 -lxcb -lxcb-keysyms -lxcb-util -lxcb-ewmh -lxcb-icccm

compile:
	$(CC) $(CFLAGS) $(LIBS) twm.c -o twm

xephyr:
	@Xephyr -br -ac -noreset -screen 1200x800 :2
