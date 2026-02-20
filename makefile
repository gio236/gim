# Nome dell'eseguibile finale
TARGET = a.out

# Regola principale
all: $(TARGET)

$(TARGET): main.cpp
	g++ main.cpp ~/gim/ncurses-toolkit/src/menu.cpp ~/gim/ncurses-toolkit/src/message_box.cpp -lncurses -o $(TARGET)


# Regola per pulire i file generati
clean:
	rm -f $(TARGET)
