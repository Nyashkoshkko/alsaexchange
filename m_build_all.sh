# 1) build libalsaexchange.so
g++ -shared alsaexchange.cc -o libalsaexchange.so `pkg-config --cflags --libs gtk+-3.0 alsa` -std=c++11

# 2) build alsaexchange.dll that's wraps libalsaexchange.so
winegcc -shared alsaexchange.dll.c alsaexchange.dll.spec -o alsaexchange.dll -L. -lalsaexchange -Wl,-rpath=.

# 3) build alsaexchange.exe that calls alsaexchange.dll that load libalsaexchange.so during loadlibrary()
wineg++ alsaexchange.exe.c -o alsaexchange
