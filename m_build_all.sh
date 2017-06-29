# 1) build libalsaexchange.so
gcc -shared alsaexchange.c -o libalsaexchange.so `pkg-config --cflags --libs gtk+-3.0`

# 2) build alsaexchange.dll that's wraps libalsaexchange.so
winegcc -shared alsaexchange.dll.c alsaexchange.dll.spec -o alsaexchange.dll -L. -lalsaexchange -Wl,-rpath=.

# 3) build alsaexchange.exe that calls alsaexchange.dll that load libalsaexchange.so during loadlibrary()
wineg++ alsaexchange.exe.c -o alsaexchange
