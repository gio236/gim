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

void printfile(const vector<string> &righe){
  for(int i = 0; i < righe.size(); i++){
    mvprintw(i, 0, "%s", righe[i].c_str());
  }
  refresh();
}

void disableFlowControl(){
  termios t;				// dichiara t per salvare la configurazione della flag 
  tcgetattr(STDIN_FILENO, &t);	// ribaltata con ~ (not in c) ribalta i bit della flag
  t.c_iflag &= ~(IXON);		// nella zona IXON che attiva l'input dei caratteri ctrl+s ecc..
  tcsetattr(STDIN_FILENO, TCSANOW, &t); // e poi lo risalvo con settattribut immediatamente con tcs.
}


int main(int argc, char *argv[]){

  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);

  vector<string> righe;

  int x ;
  int y ;
  int ch;
  bool filexist = false;

  string Nomefile ; //dichiarazione del nome del file

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

  initscr();	     //inizializzo ncurses
  noecho();	     //disattivo i tasti mostrati a schermo 
  keypad(stdscr, TRUE);//attivo tasti speciali
  cbreak();
  disableFlowControl();
  nodelay(stdscr, TRUE); //non bloccare il programma in attesa di input

  if(filexist){
    printfile(righe);
  }



  while((ch = getch()) != EXIT_KEY){//se ch diverso da ctrl+x lo faccio entrare altrimenti salta tutto e finisce il programma

    // 212 rimpiazzare con termwidth // COLS
    if(x >= COLS - 1){
      x = 0;
      y++;
      righe.push_back(""); //aggiungo una riga vuota
      move(y,x);
      refresh();
    }

    if(ch == KEY_UP){
      if(y > 0){
        y--;
        if(y < righe.size()){
          x = righe[y].length();
        }move(y, x);
      }
    }else if(ch == KEY_DOWN){
      if(y + 1 < righe.size()){
        y++;
        x = righe[y].length();
        move(y,x);
        refresh();
      }
    }else if(ch == KEY_LEFT){
      if(x > 0){
        x--;
        move(y, x);
      }else if (x == 0 && y > 0){
        y--;
        x = righe[y].length();
        move(y, x);
      }
    }else if(ch == KEY_RIGHT){
      if(x < COLS - 1 && x < righe[y].length())
        x++;

      move(y, x);
    }else if(ch == SAVE_KEY){ 
      ofstream file(Nomefile);
      if(file.is_open()){
        for(int i = 0; i < righe.size(); i++){
          file << righe[i] << endl; //scrivo il file
        }
        file.close();

        vector<string> items = {"OK", "EXIT"};
        string sel = printmenu("writing went well", items); // return the selection of the user
        resetglobalsrc();

        if(sel == "EXIT"){
          endwin();
          return 0;
        }

      }else{

        vector<string> items = {"OK", "EXIT"};
        string sel = printmenu("writing went wrong", items); // return the selection of the user
        resetglobalsrc();

        if(sel == "EXIT"){
          endwin();
          return 0;
        }

        x = 0;
        refresh();
      }
      refresh();
    }else if(ch == '\n'){
      y++;
      if(y < righe.size()){
        x = righe[y].length();	
      }else{
        righe.push_back("");	
        x = 0;
      }				
      move (y,x);
      refresh();
    }else if(ch == KEY_BACKSPACE){ //controllo se ch è backspace e tolgo un carattere
      if(x > 0){
        x--;
        //	righe[y].pop_back();
        if (y >= 0 && y < righe.size() && x >= 0 && x < righe[y].size()) {
          righe[y].erase(righe[y].begin() + x);                                             
        }

        move(y, 0);  
        clrtoeol(); // cancella tutta la riga in cui ti trovi
        mvprintw(y, 0, "%s", righe[y].c_str());
        move(y, x);
        refresh();
      }else if (x == 0 && y > 0){
        y--;
        x = righe[y].length();
        move(y, x);
        refresh();
      }
    }else if(isprint(ch)){
      righe[y].insert(x, 1, (char)ch);
      move(y, 0);
      clrtoeol();
      mvprintw(y, 0, "%s", righe[y].c_str());
      move (y, x);
      x++;
    }
  }

  endwin();

  return 0;
}
