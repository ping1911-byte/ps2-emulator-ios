.PHONY: test clean build

COMPILER = clang++
FLAGS = -std=c++17 -Wall -Wextra -O2
INCLUDE = -I.
LDFLAGS = 

TEST_EXECUTABLE = ps2_test
TEST_SOURCES = test.cpp PS2Emulator/PS2Emulator.cpp
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

build: $(TEST_OBJECTS)
	$(COMPILER) $(FLAGS) -o $(TEST_EXECUTABLE) $(TEST_OBJECTS) $(LDFLAGS)

test: build
	./$(TEST_EXECUTABLE)

%.o: %.cpp
	$(COMPILER) $(FLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(TEST_OBJECTS) $(TEST_EXECUTABLE)

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  make build  - Compile test executable"
	@echo "  make test   - Compile and run tests"
	@echo "  make clean  - Remove build artifacts"
