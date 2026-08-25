CPP = g++
AR  = ar

CFLAGS      = -Wall -Wextra -ggdb -fsanitize=undefined -fno-sanitize-recover=undefined -fsanitize=float-divide-by-zero
THIRDPARTY_CFLAGS = -ggdb

SRC_DIR     = ./src
THIRDPARTY  = ./thirdparty
DEPS_DIR    = ./deps
LIB_DIR     = $(DEPS_DIR)/lib/x86_64-linux
BUILD_DIR   = ./build

INCLUDES = -isystem $(DEPS_DIR)/include/ -isystem $(THIRDPARTY)/imgui -isystem $(THIRDPARTY)/rlImGui
LIBS     = $(LIB_DIR)/libraylib.a $(LIB_DIR)/libimgui.a $(LIB_DIR)/librlimgui.a -lm -lX11

# main
SRC          = $(SRC_DIR)/main.cpp $(SRC_DIR)/uav.cpp $(SRC_DIR)/draw.cpp $(SRC_DIR)/util.cpp
HEADER_ONLY  = $(SRC_DIR)/euler_angle.hpp $(SRC_DIR)/serial.h
OBJ_DIR      = $(BUILD_DIR)/obj
APP_OBJ      = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))

# imgui
IMGUI_SRC    = $(wildcard $(THIRDPARTY)/imgui/*.cpp)
IMGUI_OBJDIR = $(BUILD_DIR)/imgui
IMGUI_OBJ    = $(patsubst $(THIRDPARTY)/imgui/%.cpp,$(IMGUI_OBJDIR)/%.o,$(IMGUI_SRC))

# rlImGui
RLIMGUI_OBJDIR = $(BUILD_DIR)/rlimgui
RLIMGUI_OBJ    = $(RLIMGUI_OBJDIR)/rlImGui.o

DEPS = $(APP_OBJ:.o=.d) $(IMGUI_OBJ:.o=.d) $(RLIMGUI_OBJ:.o=.d)

.PHONY: all clean clean-app

all: imu_visualizer

imu_visualizer: $(APP_OBJ) $(LIB_DIR)/libimgui.a $(LIB_DIR)/librlimgui.a $(HEADER_ONLY)
	$(CPP) $(CFLAGS) $(APP_OBJ) $(LIBS) -o $@

# main obj
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CPP) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

# imgui static lib 
$(LIB_DIR)/libimgui.a: $(IMGUI_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $(IMGUI_OBJ)

$(IMGUI_OBJDIR)/%.o: $(THIRDPARTY)/imgui/%.cpp | $(IMGUI_OBJDIR)
	$(CPP) $(THIRDPARTY_CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(IMGUI_OBJDIR):
	mkdir -p $@

# rlImGui static lib 
$(LIB_DIR)/librlimgui.a: $(RLIMGUI_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $(RLIMGUI_OBJ)

$(RLIMGUI_OBJDIR)/%.o: $(THIRDPARTY)/rlImGui/%.cpp | $(RLIMGUI_OBJDIR)
	$(CPP) $(THIRDPARTY_CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(RLIMGUI_OBJDIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@


clean:
	rm -rf $(BUILD_DIR) imu_visualizer

clean-app:
	rm -rf $(OBJ_DIR) imu_visualizer

-include $(DEPS)
