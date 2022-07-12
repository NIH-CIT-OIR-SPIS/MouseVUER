CXX=g++
CXX_FLAGS= -g -Wall -fopenmp -O3 -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -std=c++17 -Ithird-party/ -Irecorder 
CXX_LFLAGS=-lrealsense2 -pthread -lglfw -lGL -lglut -lGLU -lavcodec -lavformat -lavutil -lswresample -lm -lz -lswscale
SOURCES=recorder/multicam_main.cpp recorder/util_multicam.cpp  #add your source files here
SOURCES2=decomp_dc.cpp
OBJECTS=$(SOURCES:.cpp=.o) #takes the .cpp files from the SOURCES var and replaces the .cpp with .o for each
OBJECTDIR=bin
EXEC=bin/multicam
EXEC2 =bin/decompress

all: bin/ $(SOURCES) $(EXEC) bin/decompress
	
bin:
	mkdir -p bin Testing_DIR

bin/decompress:
	$(CXX) $(CXX_FLAGS) $(SOURCES2) $(CXX_LFLAGS) `pkg-config --cflags --libs opencv4 glfw3` -o bin/decompress

clean:
	rm -f $(OBJECTS)
	rm -f $(EXEC)

$(EXEC): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(CXX_LFLAGS) `pkg-config --cflags --libs opencv4 glfw3`  #$@ will be replaced with the content of EXEC, so your applications name

#a build rule for all .cpp files to be compiled to a .o file. % is a placeholder for the actual name of the file
$(OBJECTDIR)/%.o: %.cpp
	$(CXX) $(CXX_FLAGS) $<  # $< will be replaced with the .cpp file name