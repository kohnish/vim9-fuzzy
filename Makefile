vim9-fuzzy:
	gcc $(CFLAGS) $(LDFLAGS) native/app/*.c -Inative/ext/jsmn -Os -luv -o bin/vim9-fuzzy

macports-vim9-fuzzy:
	LDFLAGS=-L/opt/local/lib CFLAGS=-I/opt/local/include $(MAKE) vim9-fuzzy

clean:
	rm bin/vim9-fuzzy
