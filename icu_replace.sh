gcc -fPIC -O2 -Wall -shared -o icu_replace.so icu_replace.c `icu-config --ldflags`
strip icu_replace.so