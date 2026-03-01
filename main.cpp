#include <fstream>
#include <string>
#include <vector>
#include <signal.h>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <ncurses.h>
#include <filesystem>

const int SAVE_KEY = 23; // ctrl+w
const int QUIT_KEY = 24; // ctrl+x
const int KEY_CTRL_LEFT = 1000; 
const int KEY_CTRL_RIGHT = 1001;
const int KEY_CTRL_UP = 1002; 
const int KEY_CTRL_DOWN = 1003; 

const std::string statusmessage = "ctrl-w for writing, ctrl-x for exit";


// TODO
// colonna desiderata
// backspace che unisce righe
// status bar
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

void desiredcols(Cursor &c, const Buffer &b){
  int len = b.rows[c.y].length();
  if(c.x > len){
    c.x = len;
  }
}

void ref(const Cursor &c, const Viewport &v, WINDOW *status, const std::string pt, const Buffer &b){
  werase(status);
  // dovrei implementare controllo per larghezza della finestra
  mvwprintw(status, 0, 0, "%s - %d/%d lines", pt.c_str(), c.y, b.rows.size());
  mvwprintw(status, 0, COLS - 1 - statusmessage.length(), "%s", statusmessage.c_str());

  move((c.y - v.firstpov), c.x);
  wrefresh(status);
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

void upmove(Cursor &c,const Buffer &b, Viewport &v){
  if(c.y - 1 >= 0){
    if((c.y - v.firstpov) == 0 && c.y > 0){
      v.firstpov--;
    }
    c.y--;
    desiredcols(c, b);
  }
}

void downmove(Cursor &c, const Buffer &b, Viewport &v){
  if(c.y + 1 < b.rows.size()){
    if((c.y - v.firstpov) == LINES - 2){
      v.firstpov++; 
    }
    c.y++;
    desiredcols(c, b);
  }
}



void leftmove(Cursor &c, const Buffer &b, Viewport &v){
  if(c.x - 1 >= 0){
    c.x--;
  }else if(c.y > 0){
    upmove(c, b, v);
    c.x = b.rows[c.y].length();
  }
}

void rightmove(Cursor &c, const Buffer &b, Viewport &v){
  if(c.x + 1 <= b.rows[c.y].length()){
    c.x++;
  }else if(c.y + 1 < b.rows.size()){
    downmove(c, b, v);
    c.x = 0;
  }
}

void writerow(Cursor &c, Buffer &b, Viewport &v, int ch){
  b.rows[c.y].insert(c.x, 1, char(ch));
  rightmove(c, b, v);
}

void removechar(Cursor &c, Buffer &b, Viewport &v){
  if(c.x > 0 && c.y >= 0 && c.y < b.rows.size()){
    leftmove(c, b, v);
    b.rows[c.y].erase(b.rows[c.y].begin() + c.x);
  }
}

void printfile(const Viewport &v, const Buffer &b){
  clear();
  int temp = 0;
  for(int i = v.firstpov; i < b.rows.size() && temp < LINES - 1; i++){ 
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
        if(b.rows.empty()){
          b.rows.push_back("");
        }
        file.close();
      }
    }else{
      b.rows.push_back("");
    }
  }else{
    std::cerr << "Usage : gim <filename>\n";
    return 1;
  }


  init();

  printfile(v, b);

  WINDOW *status = newwin(1, COLS, LINES - 1, 0); // statusbar
  if(has_colors()) {
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    wbkgd(status, COLOR_PAIR(1));
  }


  define_key("\033[1;5A", KEY_CTRL_UP);
  define_key("\033[1;5B", KEY_CTRL_DOWN);
  define_key("\033[1;5C", KEY_CTRL_RIGHT);
  define_key("\033[1;5D", KEY_CTRL_LEFT);

  ref(c, v, status, pt, b);
  ref(c, v, status, pt, b);

  // main loop

  
  while(true){

    povupdate = v.firstpov;
    ch = getch();

    if(isprint(ch)){
      writerow(c, b, v, ch);
      printrow(c, b, v);
    }else if(ch == KEY_BACKSPACE){
      removechar(c, b, v);
      printrow(c, b, v);
    }else if(ch == SAVE_KEY){
      savefile(b, pt);
    }else if(ch == KEY_LEFT){
      leftmove(c, b, v);
    }else if(ch == KEY_RIGHT){
      rightmove(c, b, v);
    }else if(ch == QUIT_KEY){
      endwin();
      return 0;
    }else if(ch == KEY_UP){
      upmove(c, b, v);
    }else if(ch == KEY_DOWN){
      downmove(c, b, v);
    }else if(ch == '\n'){
      insertline(c, b, v);
      printfile(v, b);
      ref(c, v, status, pt, b);
    }

    if(povupdate != v.firstpov){
      printfile(v, b);
      ref(c, v, status, pt, b);
    }

    ref(c, v, status, pt, b);

  }
  endwin();
  return 0;
}
