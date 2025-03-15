Current project is still under construction.
However, it can already be tested. To do so, make sure dyn and exe directories exist first.
Then, execute the following commands:

gcc -fPIC -shared -fvisibility=hidden src/MutexGuard.c -o dyn/libmtxgrd.so
gcc -g -Wall src/main.c -Ldyn -lmtxgrd -o exe/main
LD_LIBRARY_PATH=dyn ./exe/main