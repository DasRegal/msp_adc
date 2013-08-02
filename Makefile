NAMEELF=main.elf
NAMEC=main.c
all:
	msp430-gcc -Os -mmcu=msp430f2002 -o $(NAMEELF) $(NAMEC)

clean:
	rm -rf *.elf