all: imu_visualizer

CC=g++
CFLAGS=-Wall -Wextra -ggdb -fsanitize=undefined -fno-sanitize-recover=undefined -fsanitize=float-divide-by-zero
SRC_DIR=./src
SRC=$(SRC_DIR)/main.cpp $(SRC_DIR)/draw.cpp
HEADERS=$(SRC_DIR)/euler_angle.hpp

imu_visualizer: $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC) -lm -lX11 ./raylib/lib/libraylib.a  -isystem ./raylib/include -o imu_visualizer 
