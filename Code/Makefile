# CXX=g++
# CXX_FLAGS= -g -Wall -fopenmp -O3 -D__STDC_CONSTANT_MACROS 
# CXX_FLAGS += -D__STDC_LIMIT_MACROS -std=c++17 $(shell pkg-config --cflags opencv4) -Ithird-party/ -Irecorder -I/usr/local/include/ -I/usr/local/include/opencv4/ 

# CXX_LFLAGS=-lrealsense2 -pthread -lglfw -lGL -lglut -lGLU -lavcodec -lavformat -lavutil -lswresample -lm -lz -lswscale $(shell pkg-config --libs opencv4)

# SOURCES=recorder/multicam_main.cpp recorder/util_multicam.cpp  #add your source files here
# SOURCES2=decompress.cpp
# OBJECTS=$(SOURCES:.cpp=.o) #takes the .cpp files from the SOURCES var and replaces the .cpp with .o for each
# OBJECTDIR=bin
# EXEC=bin/multicam
# EXEC2 =bin/decompress

# all: bin/ $(SOURCES) $(EXEC) bin/decompress 
	
# bin:
# 	mkdir -p bin Testing_DIR




# bin/decompress:
# 	$(CXX) $(CXX_FLAGS) $(SOURCES2) $(CXX_LFLAGS) -o bin/decompress



# clean:
# 	rm -f $(OBJECTS)
# 	rm -f $(EXEC)

# $(EXEC): $(OBJECTS)
# 	$(CXX) -o $@  $(OBJECTS) $(CXX_LFLAGS)
# #$@ will be replaced with the content of EXEC, so your applications name

# #a build rule for all .cpp files to be compiled to a .o file. % is a placeholder for the actual name of the file
# $(OBJECTDIR)/%.o: %.cpp
# 	$(CXX) $(CXX_FLAGS)  $< 
	
# # $< will be replaced with the .cpp file name


CC = g++
CFLAGS = -std=c++17 $(shell pkg-config --cflags glfw3 opencv4) -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -Ithird-party -Irecorder
CFLAGS += -D__STDC_LIMIT_MACROS -fopenmp -O3 -D__STDC_CONSTANT_MACROS -Wall
LIBS = $(shell pkg-config --libs glfw3 opencv4) -lrealsense2 -pthread -lglfw -lGL -lglut -lGLU -lavcodec -lavformat -lavutil -lswresample -lm -lz -lswscale



all: bin/  bin/decompress bin/multicam

bin:
	mkdir -p bin Testing_DIR

bin/decompress: decompress.o
	${CC} -o bin/decompress decompress.o $(LIBS)

bin/multicam: multicam.o util_multicam.o
	${CC} -o bin/multicam multicam.o util_multicam.o $(LIBS)

decompress.o: decompress.cpp
	${CC} $(CFLAGS) -c decompress.cpp

multicam.o: recorder/multicam.cpp
	${CC} $(CFLAGS) -c recorder/multicam.cpp

util_multicam.o: recorder/util_multicam.cpp
	${CC} $(CFLAGS) -c recorder/util_multicam.cpp

clean:
	rm -f *.o
	rm -f bin/decompress bin/multicam