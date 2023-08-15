vim9-fuzzy:
	gcc $(CFLAGS) $(LDFLAGS) native/app/*.c -Inative/ext/jsmn -Os -luv -o bin/vim9-fuzzy

macports-vim9-fuzzy:
	LDFLAGS=-L/opt/local/lib CFLAGS=-I/opt/local/include $(MAKE) vim9-fuzzy

online-build:
	rm -rf build
	cmake -Bbuild -DBUILD_STATIC=ON
	cmake --build build --target install/strip

linux-download:
	mkdir -p bin
	curl -L https://github.com/kohnish/vim9-fuzzy/releases/download/v1.5/vim9-fuzzy-linux-x86-64 -o bin/vim9-fuzzy
	chmod +x bin/vim9-fuzzy

win-download:
	mkdir -p bin
	curl -L https://github.com/kohnish/vim9-fuzzy/releases/download/v1.5/vim9-fuzzy-win-x86-64 -o bin/vim9-fuzzy.exe
	chmod +x bin/vim9-fuzzy.exe

macos-download:
	mkdir -p bin
	curl -L https://github.com/kohnish/vim9-fuzzy/releases/download/v1.5/vim9-fuzzy-osx-x86-64 -o bin/vim9-fuzzy
	chmod +x bin/vim9-fuzzy

clean:
	rm bin/vim9-fuzzy
