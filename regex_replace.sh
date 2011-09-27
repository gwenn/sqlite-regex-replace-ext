gcc -fPIC -Wall -shared -o replace.so `pkg-config --cflags glib-2.0` regex_replace.c `pkg-config --libs glib-2.0`
