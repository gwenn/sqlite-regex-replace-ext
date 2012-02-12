gcc -fPIC -O2 -Wall -shared -o glib_replace.so `pkg-config --cflags glib-2.0` glib_replace.c `pkg-config --libs glib-2.0`
strip glib_replace.so