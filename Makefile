CC = g++
CFLAGS = -pthread -O0 -std=c++2a -mcx16

TARGET = containers

SOURCES = main.cpp trieber_stack.cpp msq.cpp my_atomics.cpp sgl.cpp elimination.cpp flat_combining.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Default rule to build all executables
all: $(TARGET)

# Rule to build executables
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executables
clean:
	rm -f $(OBJECTS) $(TARGET)
