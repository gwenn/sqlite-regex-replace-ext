gcc -fPIC -O2 -Wall -shared -o icu_replace.so icu_replace.c $(pkg-config --libs icu-i18n)
strip icu_replace.so
