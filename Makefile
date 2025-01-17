CXX=g++
GLFW_FLAGS=$(shell pkg-config --cflags glfw3)
GLFW_LIBS=$(shell pkg-config --static --libs glfw3)
GLAD_INCLUDE=-I./glad
TINYXML_INCLUDE=-I./tinyxml2
STB_INCLUDE=-I./stb_image
CXXFLAGS=-W -Wall -ggdb $(GLAD_INCLUDE) $(STB_INCLUDE) $(TINYXML_INCLUDE) $(GLFW_FLAGS)
LIBS=$(GLFW_LIBS)

all: main

gl.o: gl.c
	$(CXX) -I./glad -c -o gl.o gl.c

tinyxml2.o: ./tinyxml2/tinyxml2.cpp
	$(CXX) $(TINYXML_INCLUDE) -c -o tinyxml2.o tinyxml2/tinyxml2.cpp

main: main_glfw.o render.o world.o entity.o gl.o tinyxml2.o game.o phys.o
	$(CXX) $(CXXFLAGS) -o main main_glfw.o render.o world.o entity.o gl.o tinyxml2.o game.o phys.o $(LIBS)

