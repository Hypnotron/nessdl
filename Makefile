TARGET = $(wildcard *.cpp)

CPPFLAGS += -O2 -std=c++11 -Wno-unused-value 
LDFLAGS += 

LINUX_CC = clang++
LINUX_CPPFLAGS += -DOS_LINUX $(shell sdl2-config --cflags)
LINUX_LDFLAGS += $(shell sdl2-config --libs)

WIN_CC = x86_64-w64-mingw32-c++
WIN_CPPFLAGS += -DOS_WINDOWS
WIN_LDFLAGS += -static-libstdc++ -static-libgcc -Wl,-Bstatic -lstdc++ -lpthread \
                -Wl,-Bdynamic -lmingw32 -lSDL2main -lSDL2 

MAC_CC = /home/main/src/osxcross/target/bin/o64-clang++ 
MAC_CPPFLAGS += -DOS_MACOS 
MAC_LDFLAGS += -stdlib=libc++ -F./build -framework SDL2 -rpath @executable_path/../Frameworks 

linux: $(TARGET)
	$(LINUX_CC) -o build/nessdl $^ $(CPPFLAGS) $(LINUX_CPPFLAGS) $(LDFLAGS) $(LINUX_LDFLAGS)

windows: $(TARGET)
	$(WIN_CC) -o build/nessdl.exe $^ $(CPPFLAGS) $(WIN_CPPFLAGS) $(LDFLAGS) $(WIN_LDFLAGS) 

macos: $(TARGET)
	$(MAC_CC) -o build/Nessdl.app/Contents/MacOS/nessdl.tool $^ $(CPPFLAGS) $(MAC_CPPFLAGS) $(LDFLAGS) $(MAC_LDFLAGS)

android:
	pushd build/android; ./gradlew installDebug; popd


clean:
	rm -f build/nessdl build/nessdl.exe build/Nessdl.app/Contents/MacOS/nessdl.tool 
