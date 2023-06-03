macports-vim9-fuzzy:
	gcc native/app/*.c -Inative/ext/jsmn -I/opt/local/include /opt/local/lib/libuv.a -Os -o bin/vim9-fuzzy

linux-vim9-fuzzy:
	gcc native/app/*.c -Inative/ext/jsmn -Os -luv -o bin/vim9-fuzzy

clean:
	rm bin/vim9-fuzzy
