#gcc *.c sys/$1/*.c -I. --std=c9x -O1 -g -o quake-c9x -m32 -w -lm -lSDL2
python3 ./build_unix.py