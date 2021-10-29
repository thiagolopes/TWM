CFLAGS?=-Wall -g -O0 -std=c99 -pedantic
LIBS?=-lX11 -lxcb


compile:
	$(CC) $(CFLAGS) $(LIBS) twm.c -o twm

xephyr:
	@Xephyr -br -ac -noreset -screen 1200x800 :2
