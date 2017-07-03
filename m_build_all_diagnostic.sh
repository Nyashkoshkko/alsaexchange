# 1) build libalsaexchange.so
g++ -shared alsaexchange.cc -o libalsaexchange.so `pkg-config --cflags --libs gtk+-3.0 alsa` -std=c++11 -DUSING_GTK3 -DDEBUG_LABEL
#g++ -shared alsaexchange.cc -o libalsaexchange.so `pkg-config --cflags --libs alsa` -std=c++11

# 2) build alsaexchange.dll.so that's wraps libalsaexchange.so
winegcc -shared alsaexchange.dll.c alsaexchange.dll.spec -o alsaexchange.dll -L. -lalsaexchange -Wl,-rpath=.

# 3) build alsaexchange.exe that calls alsaexchange.dll.so that load libalsaexchange.so during loadlibrary()
wineg++ alsaexchange.exe.cpp -o alsaexchange -mwindows -lwinmm
