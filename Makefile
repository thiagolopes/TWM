CFLAGS?=-Wall -O2 -std=c99 -pedantic
LIBS?= -lxcb


compile:
	$(CC) $(CFLAGS) $(LIBS) twm.c -o twm

run:
	./twm

xephyr:
	@Xephyr -br -ac -noreset -screen 1200x800 :2
