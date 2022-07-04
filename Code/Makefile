CC := g++
CC += -g -fopenmp -O3 -Wall -std=c++17 -Ithird-party/ #-I$(HOME)/boost_1_62_0/ # -I$(HOME)/ffmpeg_build/include/ -Irecorder 
#-I/usr/local/include/opencv4
LDLIBS = -lrealsense2 -pthread -lglfw -lGL -lglut -lGLU -lavcodec -lavformat -lavutil
TARGET = recorder/multicam_main
TARGET2 = decompress_8bit_h265/decompress
#TARGET2 = recorder/encode_depth
#CFLAGS = -std=c++17 -I~/ffmpeg_sources/ffmpeg/
#`pkg-config --cflags --libs opencv4`
#`pkg-config --cflags --libs opencv4`
all: bin bin/multicam 
# bin/decompress

bin:
	mkdir -p bin Testing_DIR

bin/multicam: 
	$(CC)  $(TARGET).cpp $(LDLIBS) -o bin/multicam `pkg-config --cflags --libs opencv4 glfw3`


#bin/decompress:
#	$(CC)  $(TARGET2).cpp $(LDLIBS) -o bin/decompress `pkg-config --cflags --libs opencv4`
	

clean:
	cd bin/ && rm -f multicam 
# decompress
