CC = g++
CFLAGS = -std=c++17 $(shell pkg-config --cflags glfw3 opencv4) -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -Ithird-party -Irecorder -Ithird-party/imgui/
CFLAGS += -D__STDC_LIMIT_MACROS -fopenmp -O3 -D__STDC_CONSTANT_MACROS -Wall
LIBS = $(shell pkg-config --libs glfw3 opencv4) -lrealsense2 -pthread -lglfw -lGL -lglut -lGLU -lavcodec -lavformat -lavutil -lswresample -lm -lz -lswscale -ljsoncpp

FILENM = depth_test

all: bin  bin/$(FILENM)

bin:
	mkdir -p bin/ Testing_DIR/

# bin/$(FILENM3): $(FILENM3).o
# 	${CC} -o bin/$(FILENM3) $(FILENM3).o $(LIBS)

bin/$(FILENM): $(FILENM).o
	${CC} -o bin/$(FILENM) $(FILENM).o $(LIBS)

# $(FILENM3).o: $(FILENM3).cpp
# 	${CC} $(CFLAGS) -c $(FILENM3).cpp

$(FILENM).o: $(FILENM).cpp
	${CC} $(CFLAGS) -c $(FILENM).cpp

# $(FILENM2).o: recorder/$(FILENM2).cpp
# 	${CC} $(CFLAGS) -c recorder/$(FILENM2).cpp

clean:
	rm -f *.o
	rm -f  bin/$(FILENM)
