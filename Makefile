CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LIBS = -lsfml-graphics -lsfml-window -lsfml-system

TARGET = main
SRC = main.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
