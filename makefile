TARGET = a.out

MAIN = main.cpp
STRDEP = struct/Cursorstruct.hpp struct/Viewportstruct.hpp struct/Bufferstruct.hpp
DEFDEP = def/def.hpp
CFLAG = -lncurses



all: $(TARGET)


$(TARGET): main.cpp
	g++ $(MAIN) $(STRDEP) $(DEFDEP) $(CFLAG) -o $(TARGET)


clean:
	rm -f $(TARGET)
