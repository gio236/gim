#!/bin/bash

# Colori per i messaggi
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Iniziando l'installazione di GIM ===${NC}"

# 1. Controllo dipendenze (g++)
if ! command -v g++ &>/dev/null; then
  echo -e "${RED}Errore: g++ non è installato.${NC}"
  exit 1
fi

# 2. Compilazione
echo "Compilazione in corso..."
make clean
make

if [ $? -eq 0 ]; then
  echo -e "${GREEN}Compilazione completata con successo.${NC}"
else
  echo -e "${RED}Errore durante la compilazione.${NC}"
  exit 1
fi

# 3. Installazione in /usr/local/bin
echo "Spostamento dell'eseguibile in /usr/local/bin (richiede sudo)..."
sudo cp gim /usr/local/bin/gim
sudo chmod +x /usr/local/bin/gim

# 4. Configurazione automatica TERM (opzionale ma consigliata)
# Aggiunge l'export del terminale nel .bashrc se non presente
if ! grep -q "xterm-256color" ~/.bashrc; then
  echo "Configurazione colori nel .bashrc..."
  echo 'export TERM=xterm-256color' >>~/.bashrc
  echo -e "${GREEN}Terminale configurato a 256 colori.${NC}"
fi

echo -e "${GREEN}=== Installazione completata! ===${NC}"
echo "Ora puoi lanciare l'editor scrivendo: gim <nomefile>"
echo "Riavvia il terminale o scrivi 'source ~/.bashrc' per attivare i colori."
