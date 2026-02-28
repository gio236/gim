TARGET = a.out

all: $(TARGET)

$(TARGET): main.cpp
	g++ main.cpp -lncurses -o $(TARGET)


clean:
	rm -f $(TARGET)
