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

// colonna desiderata
// backspace che unisce righe
// secondo commento da gim btw

struct Cursor{
  int y;
  int x;
};

struct Viewport{
  int firstpov;
};

struct Buffer{
  std::vector<std::string> rows;
};

void ref(const Cursor &c, const Viewport &v){
  move((c.y - v.firstpov), c.x);
  refresh();
}

void disableFlowControl(){
  termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_iflag &= ~(IXON);
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void savefile(const Buffer &b, const std::string &pt){
  std::ofstream file(pt);
  if(file.is_open()){
    for(int i = 0; i < b.rows.size(); i++){
      file << b.rows[i] << std::endl;
    }
    file.close();
  }
}

void init(){
  initscr();         
  noecho();         
  keypad(stdscr, TRUE);
  cbreak();
  disableFlowControl();
}

void printrow(const Cursor &c, const Buffer &b, const Viewport &v){
  move((c.y - v.firstpov), 0);
  clrtoeol();
  printw("%s", b.rows[c.y].c_str());
}

void leftmove(Cursor &c){
  if(c.x - 1 >= 0){
    c.x--;
  }
}

void rightmove(Cursor &c, const Buffer &b){
  if(c.x + 1 <= b.rows[c.y].length()){
    c.x++;
  }
}

void writerow(Cursor &c, Buffer &b, int ch){
  b.rows[c.y].insert(c.x, 1, char(ch));
  rightmove(c, b);
}

void removechar(Cursor &c, Buffer &b){
  if(c.x > 0 && c.y >= 0 && c.y < b.rows.size()){
    leftmove(c);
    b.rows[c.y].erase(b.rows[c.y].begin() + c.x);
  }
}

void upmove(Cursor &c, Viewport &v){
  if(c.y - 1 >= 0){
    if((c.y - v.firstpov) == 0 && c.y > 0){
      v.firstpov--;
    }
    c.y--;
    c.x = 0;
  }
}

void downmove(Cursor &c, const Buffer &b, Viewport &v){
  if(c.y + 1 < b.rows.size()){
    if((c.y - v.firstpov) == LINES - 1){
      v.firstpov++; 
    }
    c.y++;
    c.x = 0;
  }
}

void printfile(const Viewport &v, const Buffer &b){
  clear();
  int temp = 0;
  for(int i = v.firstpov; i < b.rows.size() && temp < LINES; i++){ 
    mvprintw(temp, 0, "%s", b.rows[i].c_str());
    temp++;
  }
}

void insertline(Cursor &c, Buffer &b, Viewport &v){
  b.rows.push_back("");
  for(int i = b.rows.size() - 1; i > c.y; i--){
    b.rows[i] = b.rows[i - 1];
  }
  
  b.rows[c.y + 1] = b.rows[c.y].substr(c.x);
  b.rows[c.y] = b.rows[c.y].substr(0, c.x);
  
  downmove(c, b, v);
  c.x = 0;
}

int main(int argc, char *argv[]){

  const int SAVE_KEY = 23; // ctrl+w
  const int QUIT_KEY = 24; // ctrl+x
  const int KEY_CTRL_LEFT = 1000; 
  const int KEY_CTRL_RIGHT = 1001;
  const int KEY_CTRL_UP = 1002; 
  const int KEY_CTRL_DOWN = 1003; 

  Cursor c;
  Buffer b;
  Viewport v;

  c.x = 0;
  c.y = 0;

  int ch;
  int povupdate;

  v.firstpov = 0;

  std::string pt;

  if(argc > 1){
    pt = argv[1];
    if(std::filesystem::exists(pt)){
      std::ifstream file(pt);
      if(file.is_open()){
        std::string line;
        while(getline(file, line)){
          b.rows.push_back(line);
        }
      }
      if(b.rows.empty()){
        b.rows.push_back("");
      }
      file.close();
    }else{
      b.rows.push_back("");
    }
  }else{
    std::cerr << "you must pass an argument\n";
    return 1;
  }

  init();
  printfile(v, b);
  ref(c, v);

  define_key("\033[1;5D", KEY_CTRL_LEFT);
  define_key("\033[1;5C", KEY_CTRL_RIGHT);
  define_key("\033[1;5A", KEY_CTRL_UP);
  define_key("\033[1;5B", KEY_CTRL_DOWN);

  // main loop
  
  while(true){

    povupdate = v.firstpov;
    ch = getch();

    if(isprint(ch)){
      writerow(c, b, ch);
      printrow(c, b, v);
    }else if(ch == KEY_BACKSPACE){
      removechar(c, b);
      printrow(c, b, v);
    }else if(ch == SAVE_KEY){
      savefile(b, pt);
    }else if(ch == KEY_LEFT){
      leftmove(c);
    }else if(ch == KEY_RIGHT){
      rightmove(c, b);
    }else if(ch == QUIT_KEY){
      endwin();
      return 0;
    }else if(ch == KEY_UP){
      upmove(c, v);
    }else if(ch == KEY_DOWN){
      downmove(c, b, v);
    }else if(ch == '\n'){
      insertline(c, b, v);
      printfile(v, b);
    }

    if(povupdate != v.firstpov){
      printfile(v, b);
    }

    ref(c, v);

  }
  endwin();
  return 0;
}
