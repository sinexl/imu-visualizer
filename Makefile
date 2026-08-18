all: imu_visualizer

CC=gcc 
CFLAGS=-Wall -Wextra -g
SRC=main.c draw.c

imu_visualizer: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lm -lX11 ./raylib/lib/libraylib.a  -I./raylib/include -o imu_visualizer 
