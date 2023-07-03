# printing
V = 0
Q = $(if $(filter 1,$V),,@)
M = $(shell printf "\033[34;1m▶\033[0m")

# C/C++ 컴파일러의 경로를 raspberrypi/buildroot 의 aarch64 용 컴파일러의 경로로 변경해주어야합니다.
BUILDROOT_DIR=/home/user1/Downloads/buildroot
TOOLCHAIN_PATH=$(BUILDROOT_DIR)/output/host/bin
CC=$(TOOLCHAIN_PATH)/aarch64-buildroot-linux-gnu-gcc
CXX=$(TOOLCHAIN_PATH)/aarch64-buildroot-linux-gnu-g++

TARGET = toy_system
SYSTEM = ./system/system
UI = ./system/ui
WEB_SERVER = ./system/web_server
HAL = ./system/hal

INCLUDES = -I$(SYSTEM) -I$(UI) -I$(WEB_SERVER) -I$(HAL) -I./system/

#CC = gcc
CXXLIBS = -lpthread -lm -lrt -Wl,--no-as-needed -ldl -lseccomp
CXXFLAGS = $(INCLUDEDIRS) -g -O0 -std=c++14

objects = main.o system_server.o web_server.o input.o gui.o shared_memory.o dump_state.o hardware.o
# cxx_objects = camera_HAL.o ControlThread.o
cxx_objects =
shared_libs = system/libcamera.oem.so system/libcamera.toy.so

$(TARGET): $(objects) $(cxx_objects) $(shared_libs)
	$(CXX) -o $(TARGET) $(objects) $(cxx_objects) $(CXXLIBS)

main.o: system/main.c
	$(CC) -g $(INCLUDES) -c system/main.c

system_server.o: $(SYSTEM)/system_server.h $(SYSTEM)/system_server.c
	$(CC) -g $(INCLUDES) -c ./system/system/system_server.c

dump_state.o: $(SYSTEM)/dump_state.h $(SYSTEM)/dump_state.c
	$(CC) -g $(INCLUDES) -c ./system/system/dump_state.c

shared_memory.o: $(SYSTEM)/shared_memory.h $(SYSTEM)/shared_memory.c
	$(CC) -g $(INCLUDES) -c ./system/system/shared_memory.c

gui.o: $(UI)/gui.h $(UI)/gui.c
	$(CC) -g $(INCLUDES) -c $(UI)/gui.c

input.o: $(UI)/input.h $(UI)/input.c
	$(CC) -g $(INCLUDES) -c $(UI)/input.c

web_server.o: $(WEB_SERVER)/web_server.h $(WEB_SERVER)/web_server.c
	$(CC) -g $(INCLUDES) -c $(WEB_SERVER)/web_server.c

hardware.o: $(HAL)/hardware.c
	$(CC) -g $(INCLUDES) -c  $(HAL)/hardware.c

.PHONY: libcamera.oem.so
libcamera.oem.so:
	$(CC) -g -shared -fPIC -o libcamera.oem.so $(INCLUDES) $(CXXFLAGS) $(HAL)/oem/camera_HAL_oem.cpp $(HAL)/oem/ControlThread.cpp

.PHONY: libcamera.toy.so
libcamera.toy.so:
	$(CC) -g -shared -fPIC -o libcamera.toy.so $(INCLUDES) $(CXXFLAGS) $(HAL)/toy/camera_HAL_toy.cpp $(HAL)/toy/ControlThread.cpp

# libcamera.toy.so:
# 	$(CC) -g -shared -fPIC -o libfoo2.so foo2.c

# libfoo3.so:
# 	cc -g -Wl,-Bsymbolic -Wl,-allow-shlib-undefined \
# 		-shared -fPIC -o libfoo3.so foo3.

# camera_HAL.o: $(HAL)/camera_HAL.cpp
# 	$(CXX) -g $(INCLUDES) $(CXXFLAGS) -c  $(HAL)/camera_HAL.cpp

# ControlThread.o: $(HAL)/ControlThread.cpp
# 	$(CXX) -g $(INCLUDES) $(CXXFLAGS) -c  $(HAL)/ControlThread.cpp

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf $(TARGET)

## build: build toy-web
.PHONY: toy-web
toy-web: | ; $(info $(M) building arm backend…)
	cd ans-embedded && env GOARM=8 CGO_ENABLED=1 GOARCH=arm GOOS=linux GOGC=off GO111MODULE=on  $(GO) build -ldflags '$(LDFLAGS)' -o toy-web