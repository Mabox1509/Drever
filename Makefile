# Variables
CXX := g++
CXXFLAGS := -Wall -Wextra -O0 
LDFLAGS := -lz -llua5.4 -lm -lpthread
EXECUTABLE := bin/drever

# Busca todos los .cpp en src/ y sus subdirectorios
SRC_DIR := src
BUILD_DIR := out
BIN_DIR := bin

SOURCES := $(shell find $(SRC_DIR) -type f -name "*.cpp")
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES))

# Reglas
.PHONY: all clean run

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Crea los directorios si no existenma
$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE)

run: all
	./$(EXECUTABLE)
