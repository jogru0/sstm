CXX      	 := g++
CXXFLAGS 	 := -MD -Wall -Wextra -std=c++20 -Wfatal-errors -Wall -Wextra -Wshadow -Wconversion -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wsign-conversion -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2 -Woverloaded-virtual -Wno-null-dereference -pedantic -Wswitch-enum

LDFLAGS  	 := -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lboost_serialization
BUILD    	 := ./build
TARGET   	 := turning

OBJ_DIR_DBG  := $(BUILD)/$(TARGET)-debug/objects
OBJ_DIR_TST  := $(BUILD)/$(TARGET)-test/objects
OBJ_DIR_REL  := $(BUILD)/$(TARGET)-release/objects

APP_DIR  	 := ./bin
INCLUDE  	 := -iquote include -isystem submodules -isystem external_header
SRC      	 := $(wildcard src/*.cpp)

OBJECTS_DBG := $(SRC:%.cpp=$(OBJ_DIR_DBG)/%.o)
OBJECTS_TST := $(SRC:%.cpp=$(OBJ_DIR_TST)/%.o)
OBJECTS_REL := $(SRC:%.cpp=$(OBJ_DIR_REL)/%.o)


all: test

build_binary_debug: build $(APP_DIR)/$(TARGET)-debug
build_binary_test: build $(APP_DIR)/$(TARGET)-test
build_binary_release: build $(APP_DIR)/$(TARGET)-release

$(OBJ_DIR_DBG)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(OBJ_DIR_TST)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(OBJ_DIR_REL)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $<

$(APP_DIR)/$(TARGET)-debug: $(OBJECTS_DBG)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET)-debug $(OBJECTS_DBG) $(LDFLAGS)

$(APP_DIR)/$(TARGET)-test: $(OBJECTS_TST)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET)-test $(OBJECTS_TST) $(LDFLAGS)

$(APP_DIR)/$(TARGET)-release: $(OBJECTS_REL)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(APP_DIR)/$(TARGET)-release $(OBJECTS_REL) $(LDFLAGS)

.PHONY: all build clean debug release

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR_REL)
	@mkdir -p $(OBJ_DIR_DBG)
	@mkdir -p $(OBJ_DIR_TST)

debug: CXXFLAGS += -g -ggdb
debug:
	@g++ -isystem /usr/include/freetype2 -fcoroutines -MD -Wall -Wextra -std=c++20 -g -ggdb -DDEBUG -Wfatal-errors -Wall -Wextra -Wshadow -Wconversion -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wsign-conversion -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2 -Woverloaded-virtual -Wno-null-dereference -pedantic -Wswitch-enum -iquote include -isystem submodules -isystem external_header -o ./bin/sstm-debug src/main.cpp src/stb_image.cpp src/glad.c -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lassimp -lboost_serialization -lfreetype

release: CXXFLAGS += -O3 -DNDEBUG -Wno-unused-but-set-variable -Wno-unused-parameter
release:
	@g++ -isystem /usr/include/freetype2 -fcoroutines -MD -Wall -Wextra -std=c++20 -O3 -DNDEBUG -Wfatal-errors -Wall -Wextra -Wshadow -Wconversion -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wsign-conversion -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2 -Woverloaded-virtual -Wno-null-dereference -pedantic -Wswitch-enum -O3 -iquote include -isystem submodules -isystem external_header -o ./bin/sstm-release src/main.cpp src/stb_image.cpp src/glad.c -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lassimp -lboost_serialization -lfreetype

test: CXXFLAGS += -O3
test:
	@g++ -isystem /usr/include/freetype2 -fcoroutines -MD -Wall -Wextra -std=c++20 -O3 -Wfatal-errors -Wall -Wextra -Wshadow -Wconversion -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wsign-conversion -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2 -Woverloaded-virtual -Wno-null-dereference -pedantic -Wswitch-enum -O3 -iquote include -isystem submodules -isystem external_header -o ./bin/sstm-test src/main.cpp src/stb_image.cpp src/glad.c -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lassimp -lboost_serialization -lfreetype

clean:
	-@rm -rvf $(OBJ_DIR_REL)/*
	-@rm -rvf $(OBJ_DIR_DBG)/*
	-@rm -rvf $(OBJ_DIR_TST)/*
	-@rm -rvf $(APP_DIR)/*

docker:
	docker build --no-cache .

-include $(OBJECTS_REL:.o=.d)
-include $(OBJECTS_DBG:.o=.d)
-include $(OBJECTS_TST:.o=.d)
