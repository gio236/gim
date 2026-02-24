#include <fstream>
#include <string>
#include <vector>
#include <signal.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <ncurses.h>
#include <filesystem>

#include "ncurses-toolkit/include/menu.hpp"
// #include "ncurses-toolkit/include/dialog_box.hpp"
// primo commento da gim btw

using namespace std;

#define EXIT_KEY 24 // ctrl+x = 24
#define SAVE_KEY 23 // ctrl+w = 23 , ctrl+o = 15

// implementare canc
// implementare scrolling sia verticale che orizzontale

void ref(int y ,int x){
  move(y, x);
  refresh();
}

void resetglobalsrc(){
  curs_set(1);          // ripristina visibilità
  touchwin(stdscr);     // forza redraw
  refresh();
}

string printmenu(string title, vector<string> &items){
  string selection;
  try {
    Menu* menu = new Menu(title, items);
    selection = menu->show();
    delete menu;
  } catch(runtime_error &e) {
    cerr << e.what() << std::endl;
  }
  return selection;
}

void printfile(int start, const vector<string> &righe){
  int dif = 0;
  for(int i = start; i < righe.size(); i++){
    mvprintw(dif, 0, "%s", righe[i].c_str());
    dif++;
  }
  refresh();
}

void disableFlowControl(){
  termios t;                            // dichiara t per salvare la configurazione della flag
  tcgetattr(STDIN_FILENO, &t);  // ribaltata con ~ (not in c) ribalta i bit della flag
  t.c_iflag &= ~(IXON);         // nella zona IXON che attiva l'input dei caratteri ctrl+s ecc..
  tcsetattr(STDIN_FILENO, TCSANOW, &t); // e poi lo risalvo con settattribut immediatamente con tcs.
}


int main(int argc, char *argv[]){

  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);

  vector<string> righe;

  int x;
  int y;
  int ch;
  bool filexist = false;
  bool run = true;

  string Nomefile;

  x = 0;
  y = 0;


  if(argc < 2){
    cerr << "you must give an argument" << endl;
    return 1;
  }else{
    Nomefile = argv[1];
    int tmp = 0;
    if (filesystem::exists(Nomefile)){
      ifstream file(Nomefile);
      if(file.is_open()){
        string line;
        while(getline(file, line)){
          tmp++;
          righe.push_back(line);
        }
        file.close();

        filexist = true;
        if(tmp == 0)
          righe.push_back("");

      }
    }else{
      righe.push_back("");
    }


  }

  initscr();         //inizializzo ncurses
  noecho();          //disattivo i tasti mostrati a schermo
  keypad(stdscr, TRUE);//attivo tasti speciali
  cbreak();
  disableFlowControl();
  nodelay(stdscr, TRUE); //non bloccare il programma in attesa di input


  if(filexist){
    printfile(0, righe);
  }


  int my, mx;

  // mvprintw(LINES -1 , COLS - 20, "%d",LINES - 1);// debug
  // (ch = getch()) != EXIT_KEY
  while(true){
    ch = getch();
    getyx(stdscr, my, mx);
    // mvprintw(0 , COLS - 20, "y ==> %d x ==> %d",y,x );// debug
    // mvprintw(1 , COLS - 20, "my ==> %d",my );// debug

    if(x >= COLS - 1){
      ref(y, x);
    }

    if(ch == KEY_UP){
      if(y - 1 >= 0){
        y--;

        if(y < righe.size()){
          x = righe[y].length();
          if(x > COLS - 1)
            x = 0;
        }

        if(my == 0){
          clear();
          printfile(y, righe);
          ref(my, x);
        }else{
          ref(my - 1, x);
        }

      }
    }else if(ch == KEY_DOWN){
      if(y + 1 < righe.size()){
        y++;

        x = righe[y].length();
        if(x > COLS - 1)
          x = 0;

        if(my == LINES - 1){
          if(y > LINES - 1){
            clear();
            printfile(y - my, righe);
          }          
          ref(y, x);
        }else{
          ref(my + 1, x);
        }

      }
    }else if(ch == KEY_LEFT){
      if(x > 0){
        x--;
      }else if(x == 0 && y > 0){
        y--;
        my--;
        x = righe[y].length();
      }
      if(y > LINES - 1)
        ref(my, x);
      else
        ref(y, x);
    }else if(ch == KEY_RIGHT){
      if(x + 1 < righe[y].length()){
        x++;
        ref(y, x);
      }

      if(y > LINES - 1)
        ref(my, x);
      else
        ref(y, x);

    }else if(ch == SAVE_KEY){
      ofstream file(Nomefile);
      if(file.is_open()){
        for(int i = 0; i < righe.size(); i++){
          file << righe[i] << endl; //scrivo il file
        }
        file.close();

        vector<string> items = {"OK", "EXIT"};
        string sel = printmenu("writing went well", items); // return the selection of the user

        if(sel == "EXIT"){
          endwin();
          return 0;
        }

        resetglobalsrc();
      }else{

        vector<string> items = {"OK", "EXIT"};
        string sel = printmenu("writing went wrong", items); // return the selection of the user
        resetglobalsrc();

        if(sel == "EXIT"){
          endwin();
          return 0;
        }

        x = 0;
        ref(y, x);
      }
    }else if(ch == '\n'){ //enter
      y++;
      righe.push_back("");
      for(int i = righe.size() - 1; i > y; i--){
        righe[i] = righe[i - 1];
      }
      righe[y] = "";
      x = 0;
      clear();
      if(y > LINES - 1){
        //voglio scorrere il file
        // printfile(-((LINES - 1) - y),righe);
        printfile(y - my, righe);
        ref(my, x);
      }else{
        printfile(0, righe);
        ref(y, x);
      }
    }else if(ch == KEY_BACKSPACE){ //controllo se ch è backspace e tolgo un carattere
      if(x > 0){
        x--;
        if (y >= 0 && y < righe.size() && x >= 0 && x < righe[y].size()) {
          righe[y].erase(righe[y].begin() + x);
        }

        if(y > LINES - 1){
          move(my, 0);
          clrtoeol(); // cancella tutta la riga in cui ti trovi
          mvprintw(my, 0, "%s", righe[y].c_str());
          ref(my, x);
        }else{
          move(y, 0);
          clrtoeol(); // cancella tutta la riga in cui ti trovi
          mvprintw(y, 0, "%s", righe[y].c_str());
          ref(y, x);
        }

      }else if (x == 0 && y > 0){

        if (righe[y].empty()) {
          x = righe[y - 1].length();
          righe.erase(righe.begin() + y);
          clear();
          if(y > LINES - 1){
            printfile(y - my, righe);
            ref(my - 1, x);
          }else{
            printfile(0, righe);
            ref(y - 1, x);
          }
        } 

        y--;
      }
    }else if(isprint(ch)){
      righe[y].insert(x, 1, (char)ch);
      move(y, 0);
      clrtoeol();

      if(y > LINES - 1){
        mvprintw(my, 0, "%s", righe[y].c_str()); 
        //editor prevede sempre cursore alla fine del terminale
      }else{
        mvprintw(y, 0, "%s", righe[y].c_str());
      }
      
      x++;
      if(y > LINES - 1)
        ref(my, x);
      else
        ref(y, x);
    }else if(ch == EXIT_KEY){
      vector<string> items = {"YES", "NO"};
      string selection = printmenu("EXIT",items);

      if(selection == "YES"){
        endwin();
        return 0;
      }

      resetglobalsrc();
    }

    timeout(100);

  }


  endwin();

  return 0;
}

