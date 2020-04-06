TARGET = $(wildcard *.cpp)

CPPFLAGS += -std=c++11
LDFLAGS = 

LINUX_CC = clang++
LINUX_CPPFLAGS += $(shell sdl2-config --cflags)
LINUX_LDFLAGS += $(shell sdl2-config --libs)

WIN_CC = x86_64-w64-mingw32-c++
WIN_CPPFLAGS += 
WIN_LDFLAGS += -static-libstdc++ -static-libgcc -Wl,-Bstatic -lstdc++ -lpthread \
                -Wl,-Bdynamic -lmingw32 -lSDL2main -lSDL2 

MAC_CC = o64-clang++
MAC_CPPFLAGS += 
MAC_LDFLAGS += -F./build -framework SDL2 -rpath @executable_path/../Frameworks 

linux: $(TARGET)
	$(LINUX_CC) -o build/nessdl $^ $(CPPFLAGS) $(LINUX_CPPFLAGS) $(LDFLAGS) $(LINUX_LDFLAGS)

windows: $(TARGET)
	$(WIN_CC) -o build/nessdl.exe $^ $(CPPFLAGS) $(WIN_CPPFLAGS) $(LDFLAGS) $(WIN_LDFLAGS)

macos: $(TARGET)
	$(MAC_CC) -o build/Nessdl.app/Contents/MacOS/nessdl.tool $^ $(CPPFLAGS) $(MAC_CPPFLAGS) $(LDFLAGS) $(MAC_LDFLAGS)

clean:
	rm -f build/nessdl build/nessdl.exe build/Nessdl.app/Contents/MacOS/nessdl.tool 
