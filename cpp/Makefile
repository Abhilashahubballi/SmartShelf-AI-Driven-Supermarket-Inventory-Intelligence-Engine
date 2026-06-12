CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = smartshelf
SRCS = main.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
