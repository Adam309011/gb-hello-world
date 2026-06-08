CC = lcc
CFLAGS = -Wa-l -Wl-m -Wl-j
all: game.gb
game.gb: huge_rpg.c
	$(CC) $(CFLAGS) -o game.gb huge_rpg.c
clean:
	rm -f game.gb *.o *.lst *.map