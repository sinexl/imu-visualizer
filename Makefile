all: imu_visualizer

CC=gcc 
CFLAGS=-Wall -Wextra -ggdb
SRC_DIR=./src
SRC=$(SRC_DIR)/main.c $(SRC_DIR)/draw.c

imu_visualizer: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -lm -lX11 ./raylib/lib/libraylib.a  -I./raylib/include -o imu_visualizer 
