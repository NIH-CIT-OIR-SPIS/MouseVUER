# CXX=g++
# CXX_FLAGS= -g -Wall -fopenmp -O3 -D__STDC_CONSTANT_MACROS 
# CXX_FLAGS += -D__STDC_LIMIT_MACROS -std=c++17 $(shell pkg-config --cflags opencv4) -Ithird-party/ -Irecorder -I/usr/local/include/ -I/usr/local/include/opencv4/ 

# CXX_LFLAGS=-lrealsense2 -pthread -lglfw -lGL -lglut -lGLU -lavcodec -lavformat -lavutil -lswresample -lm -lz -lswscale $(shell pkg-config --libs opencv4)

# SOURCES=recorder/multicam_main.cpp recorder/$(FILENM2).cpp  #add your source files here
# SOURCES2=$(FILENM3).cpp24 GB / 60
# OBJECTS=$(SOURCES:.cpp=.o) #takes the .cpp files from the SOURCES var and replaces the .cpp with .o for each
# OBJECTDIR=bin
# EXEC=bin/$(FILENM)
# EXEC2 =bin/$(FILENM3)

# all: bin/ $(SOURCES) $(EXEC) bin/$(FILENM3) 
	
# bin:
# 	mkdir -p bin Testing_DIR




# bin/$(FILENM3):
# 	$(CXX) $(CXX_FLAGS) $(SOURCES2) $(CXX_LFLAGS) -o bin/$(FILENM3)



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

FILENM = multicam
FILENM2 = util_multicam
FILENM3 = decompress

all: bin/  bin/$(FILENM3) bin/$(FILENM)

bin:
	mkdir -p bin/ Testing_DIR/

bin/$(FILENM3): $(FILENM3).o
	${CC} -o bin/$(FILENM3) $(FILENM3).o $(LIBS)

bin/$(FILENM): $(FILENM).o $(FILENM2).o
	${CC} -o bin/$(FILENM) $(FILENM).o $(FILENM2).o $(LIBS)

$(FILENM3).o: $(FILENM3).cpp
	${CC} $(CFLAGS) -c $(FILENM3).cpp

$(FILENM).o: recorder/$(FILENM).cpp
	${CC} $(CFLAGS) -c recorder/$(FILENM).cpp

$(FILENM2).o: recorder/$(FILENM2).cpp
	${CC} $(CFLAGS) -c recorder/$(FILENM2).cpp

clean:
	rm -f *.o
	rm -f bin/$(FILENM3) bin/$(FILENM)