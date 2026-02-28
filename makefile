# Nome dell'eseguibile finale
TARGET = a.out

# Regola principale
all: $(TARGET)

$(TARGET): main.cpp
	g++ main.cpp -lncurses -o $(TARGET)


# Regola per pulire i file generati
clean:
	rm -f $(TARGET)
