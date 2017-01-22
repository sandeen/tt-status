
default: tt-status

tt-status: tt-status.c
	gcc -lmodbus -o $@ $<

clean:
	rm tt-status
