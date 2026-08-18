all: imu_visualizer

CC=gcc 
CFLAGS=-Wall -Wextra -g

imu_visualizer: main.c 
	$(CC) $(CFLAGS) main.c -lm -lX11 ./raylib/lib/libraylib.a  -I./raylib/include -o imu_visualizer 
