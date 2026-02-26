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

#define KEY_CTRL_LEFT  1001
#define KEY_CTRL_RIGHT 1002
#define EXIT_KEY 24 // ctrl+x = 24
#define TAB 2 
#define SAVE_KEY 23 // ctrl+w = 23 , ctrl+o = 15


void scrollrow(int my, int y, int x, vector<string> righe){
  move(my, 0);
  clrtoeol();
  int temp = 0;
  for(int i = x; i < righe[y].length() && temp < COLS - 1; i++){
    mvprintw(my, temp, "%c", righe[y][i]);
    temp++;
  }
}

int mvtonxtsp(int y, int my, int x, int mx, vector<string> righe){
  int i;

  if(x >= righe[y].length())
    return x;

  for(i = x + 1; i + 1 < righe[y].length() && righe[y][i] != ' '; i++){
  }

  // mvprintw(3, COLS - 20, "calcolo ==> %d", ((x + ((i - (x + 1)) + 2)) - mx + (COLS - 1)));
  // mvprintw(3, COLS - 30, "i ==> %d, calcolo ==> %d", i, ((x + ((i - (x + 1)) + 2)) - mx + (COLS - 1)));

  if(i > ((x + ((i - (x + 1)) + 2)) - mx + (COLS - 1))){
    return x;
  }else{
     return i + 1;
  }
}

int mvtoprvsp(int y, int my, int x, int &mx, vector<string> righe){
  int i;

  for(i = x - 1; i - 1 >= 0 && righe[y][i] != ' '; i--){
  }

  // mvprintw(3, COLS - 20, "calcolo ==> %d", ((x + ((i - (x - 1))) - 1)  - mx));

  if(i < ((x + ((i - (x - 1)) - 1))  - mx) || i < 0){
    return x;
  }else{
    return i;
  }

}


void reprintrow(int y, int my, vector<string> righe){
  move(my, 0);
  clrtoeol();
  mvprintw(my, 0, "%s",righe[y].c_str());
}

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
  for(int i = start; i < righe.size() && dif < LINES; i++){
    if(righe[i].length() > COLS - 1  ){
      for(int j = 0; j < righe[i].length(); j++){
        mvprintw(dif, j, "%c", righe[i][j]);
      }
      dif++;
    }else{
      mvprintw(dif, 0, "%s", righe[i].c_str());
      dif++;
    }
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

  int my, mx;

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


  define_key("\033[1;5D", KEY_CTRL_LEFT);
  define_key("\033[1;5C", KEY_CTRL_RIGHT);

  if(filexist){
    printfile(0, righe);
  }


  ref(y, x);

  // mvprintw(LINES -1 , COLS - 20, "%d",LINES - 1);// debug

  while(true){
    ch = getch();
    getyx(stdscr, my, mx);
    mvprintw(0 , COLS - 20, "y ==> %d x ==> %d",y,x );// debug
    mvprintw(1 , COLS - 20, "mx ==> %d",mx );// debug
    // mvprintw(1 , COLS - 20, "my ==> %d",my );// debug
    // mvprintw(2, COLS - 20, "%d",COLS );// debug
    // mvprintw(4, COLS - 20, "%d",COLS - 1 );// debug

    ref(my, mx); // debug

    if(ch == KEY_CTRL_RIGHT){
      mx += (mvtonxtsp(y, my, x, mx, righe) - x);
      x = mvtonxtsp(y, my, x, mx, righe);
      ref(my, mx);
    }else if(ch == KEY_CTRL_LEFT){ 
      mx += (mvtoprvsp(y, my, x, mx, righe) - x);
      x = mvtoprvsp(y, my, x, mx, righe);
      ref(my, mx);
    }else if(ch == KEY_UP){
      if(y - 1 >= 0){
        y--;

        if(y < righe.size()){
          x = righe[y].length();
          if(x > COLS - 1){
            x = 0;
          }
        }

        mx = x;

        if(my == 0){
          clear();
          printfile(y, righe);
          ref(my, mx);
        }else{
          ref(my - 1, mx);
        }

      }
    }else if(ch == KEY_DOWN){
      if(y + 1 < righe.size()){
        y++;

        x = righe[y].length();
        if(x > COLS - 1){
          x = 0;
        }

        mx = x;

        if(my == LINES - 1){
          if(y > LINES - 1){
            clear();
            printfile(y - my, righe);
          }          
          ref(my, mx);
        }else{
          ref(my + 1, x);
        }

      }
    }else if(ch == KEY_LEFT){
      if(x > 0){

        if(righe[y].length() > COLS - 1 && mx <= 0 && x >= COLS - 1){
          scrollrow(my, y, (x) - (COLS - 1), righe);
          mx = COLS - 1;
        }

        x--;
        mx--;
     }else if(x == 0 && y > 0){
        y--;
        my--;
        x = righe[y].length();

        if(righe[y].length() > COLS - 1 && mx == 0 && x >= COLS - 1){
          scrollrow(my, y,((x/(COLS - 1)) * (COLS - 1)), righe);
          mx = ( x - ((x/(COLS - 1)) * (COLS - 1)));
        }

      }

      ref(my, mx);

    }else if(ch == KEY_RIGHT){
      if(x + 1 <= righe[y].length()){

        if(righe[y].length() > COLS - 1 && mx >= COLS - 1){
          scrollrow(my, y, x, righe);
          mx = 0;
        }

        x++;
        mx++;
        ref(my, mx);
      }
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
        mx = 0;
        ref(my, mx);
      }
    }else if(ch == '\n'){ //enter
      y++;
      my++;
      righe.push_back("");
      for(int i = righe.size() - 1; i > y; i--){
        righe[i] = righe[i - 1];
      }
      righe[y] = "";
      x = 0;
      mx = 0;
      clear();
      printfile(y - my, righe);
      ref(my, mx);

    }else if(ch == KEY_BACKSPACE){ 
      if(x > 0){
        x--;
        mx--;
        if(y >= 0 && y < righe.size() && x >= 0 && x < righe[y].size()) {
          righe[y].erase(righe[y].begin() + x);
        }

        if(x > COLS - 1 || righe[y].length() > COLS - 1){
          if(righe[y].length() > COLS - 1 && x < COLS - 1){
            mx = x;
            scrollrow(my, y, 0, righe);
          }else{
            scrollrow(my, y, x - mx, righe);
          }
        }else{
          mx = x;
          reprintrow(y, my, righe);
        }
        ref(my, mx);

      }else if (x == 0 && y > 0){
        if (righe[y].empty()) {
          x = righe[y - 1].length();
          righe.erase(righe.begin() + y);
          clear();
          printfile(y - my, righe);
          ref(my - 1, x);
          y--;
        } 
      }
    }else if(isprint(ch)){

      righe[y].insert(x, 1, (char)ch);
      if(x > COLS - 1 || righe[y].length() > COLS - 1){
        scrollrow(my, y, x - mx, righe);
      }else{
        reprintrow(y, my, righe);
      }
      x++;
      mx++;
      ref(my, mx);

    }else if(ch == EXIT_KEY){
      vector<string> items = {"YES", "NO"};
      string selection = printmenu("EXIT",items);

      if(selection == "YES"){
        endwin();
        return 0;
      }

      resetglobalsrc();
    }else if(ch == 9){ // tab
      righe[y].insert(x, TAB, ' ');
      reprintrow(y, my, righe);
      mx += TAB;
      x += TAB;
      ref(my, mx);
    }

    timeout(100);

  }


  endwin();

  return 0;
}

