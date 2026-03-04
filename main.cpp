#include <fstream>
#include <cstring>
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

#define TABSPACE 4 // the number of space inserted if you press tab 
#define DEFMESS "ctrl-w for writing, ctrl-x for exit" // the default messagge in the bottombar
#define QUITIME 2 // times you want press before quit, with unsaved changes

std::string statusmessage = DEFMESS;

struct Cursor{
  int y;
  int x, mx; 
  // x is for acess to the vector
  // mx is the cursor position in the screen
};

struct Viewport{
  int firstpov;
};

struct Buffer{
  std::vector<std::string> rows;
  int dirt;
  int time = QUITIME;
};

void desiredcols(Cursor &c, const Buffer &b){
  int len = b.rows[c.y].length();
  if(c.x > len){
    c.x = len;
  }

  c.mx = 0;
  for(int i = 0; i < c.x; i++){
    if(b.rows[c.y][i] == '\t'){
      c.mx += TABSPACE - (c.mx % TABSPACE);
    }else{
      c.mx++;
    }
  }

}

void setmessage(const Cursor &c, WINDOW *status, const std::string pt, const Buffer &b){
  werase(status);
  mvwprintw(status, 0, 0, "%s - %d/%d lines", pt.c_str(), c.y + 1, b.rows.size());
  mvwprintw(status, 0, COLS - 1 - statusmessage.length(), "%s", statusmessage.c_str());
  statusmessage = DEFMESS;
}

void ref(const Cursor &c, const Viewport &v, WINDOW *status, const std::string &pt, const Buffer &b){
  setmessage(c, status, pt, b);

  refresh();
  wrefresh(status);
  move((c.y - v.firstpov), c.mx);
}

void disableFlowControl(){
  termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_iflag &= ~(IXON);
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void savefile(Buffer &b, const std::string &pt){
  std::ofstream file(pt);
  if(file.is_open()){
    for(int i = 0; i < b.rows.size(); i++){
      file << b.rows[i] << std::endl;
    }
    file.close();
    b.dirt = 0;
    b.time = QUITIME;
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
  int row = c.y - v.firstpov;
  move(row, 0);
  clrtoeol();
  int screen_col = 0;
  for(int i = 0; i < b.rows[c.y].length() && screen_col < COLS - 1; i++){
    if(b.rows[c.y][i] == '\t'){
      int spaces = (TABSPACE - (screen_col % TABSPACE));
      for(int s = 0; s < spaces && screen_col < COLS - 1; s++){
        mvaddch(row, screen_col, ' ');
        screen_col++;
      }    
    }else{
      mvaddch(row, screen_col, b.rows[c.y][i]);
      screen_col++;
    }
  }
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
    desiredcols(c, b);
  }else if(c.y > 0){
    upmove(c, b, v);
    c.x = b.rows[c.y].length();
    c.mx = b.rows[c.y].length();
  }
}

void rightmove(Cursor &c, const Buffer &b, Viewport &v){
  if(c.x + 1 <= b.rows[c.y].length()){
    c.x++;
    desiredcols(c, b);
  }else if(c.y + 1 < b.rows.size()){
    downmove(c, b, v);
    c.x = 0;
    c.mx = 0;
  }
}

void writerow(Cursor &c, Buffer &b, Viewport &v, int ch){
  b.rows[c.y].insert(c.x, 1, char(ch));
  b.time = QUITIME; 
  b.dirt++;
  rightmove(c, b, v);
}

bool removechar(Cursor &c, Buffer &b, Viewport &v){
  bool refresh = true; 
  // if refresh is returned as true 
  // this will ensure that the scren will be cleared 
  // so will print the all file 

  if(c.x > 0 && c.y >= 0){
    leftmove(c, b, v);
    b.rows[c.y].erase(b.rows[c.y].begin() + c.x);
    refresh = false;
  }else if(c.y > 0){
    c.x = b.rows[c.y - 1].length();
    c.mx = b.rows[c.y - 1].length();
    b.rows[c.y - 1] += b.rows[c.y];
    b.rows.erase(b.rows.begin() + c.y);
    upmove(c, b, v);
  }else if(b.rows[0].empty() && b.rows.size() > 1){
    b.rows.erase(b.rows.begin() + 0);
  }else{
    return false;
  }

  b.time = QUITIME; 
  b.dirt++;

  return refresh;
}

void printfile(const Viewport &v, const Buffer &b){
  clear();
  int temp = 0;

  for(int i = v.firstpov; i < b.rows.size() && temp < LINES - 1; i++){ 
    int screen_col = 0;  // colonna effettiva sullo schermo
    for(int buf_index = 0; buf_index < b.rows[i].length() && screen_col < COLS - 1; buf_index++){
      char ch = b.rows[i][buf_index];

      if(ch == '\t'){
        int spaces = TABSPACE - (screen_col % TABSPACE);
        for(int s = 0; s < spaces && screen_col < COLS - 1; s++){
          mvaddch(temp, screen_col, ' ');
          screen_col++;
        }
      }else{
        mvaddch(temp, screen_col, ch);
        screen_col++;
      }
    }
    temp++;
  }
}

void insertline(Cursor &c, Buffer &b, Viewport &v){
  b.time = QUITIME; 
  b.dirt++;

  b.rows.insert(b.rows.begin() + c.y + 1, b.rows[c.y].substr(c.x));
  b.rows[c.y] = b.rows[c.y].substr(0, c.x);

  downmove(c, b, v);
  c.x = 0;
  c.mx = 0;
}

void openfile(std::string filename, Buffer &b){
  std::ifstream file(filename);
  if(file.is_open()){
    std::string line;
    while(getline(file, line)){
      b.rows.push_back(line);
    }
    if(b.rows.empty()){
      b.rows.push_back("");
    }
    file.close();
  }else{
    std::cerr << "erorr while opening file\n";
    if(b.rows.empty())
      b.rows.push_back("");
  }
} 

void handletab(Cursor &c, Buffer &b, Viewport &v){
  b.time = QUITIME; 
  b.dirt++;
  b.rows[c.y].insert(c.x, 1, '\t');
  rightmove(c, b, v);
}

Cursor c;
Buffer b;
Viewport v;


int ch;
int povupdate;
std::string pt;


void handleinput(WINDOW *status){
  switch(ch){
    case '\t':
      handletab(c, b, v);
      printrow(c, b, v);
      break;
    case KEY_BACKSPACE: 
      if(removechar(c, b, v)){
        printfile(v, b);
      }else{
        printrow(c, b, v);
      }
      break;
    case SAVE_KEY: 
      savefile(b, pt);
      break;
    case KEY_LEFT:
      leftmove(c, b, v);
      break;
    case KEY_RIGHT:
      rightmove(c, b, v);
      break;
    case QUIT_KEY:
      if(b.dirt && b.time){
        char buffer[100];
        std::snprintf(buffer, sizeof(buffer), "WARNING you have unsaved change press ctrl-x %d more times for quit", b.time);
        statusmessage.assign(buffer);
        b.time--;
      }else{
        delwin(status);
        endwin();
        exit(0);
      }
      break;
    case KEY_UP:
      upmove(c, b, v);
      break;
    case KEY_DOWN:
      downmove(c, b, v);
      break;
    case '\n':
      insertline(c, b, v);
      printfile(v, b);
      break;
    default: 
      if(isprint(ch)){
        writerow(c, b, v, ch);
        printrow(c, b, v);
      }
  }
}

int main(int argc, char *argv[]){

  c.x = 0;
  c.mx = 0;
  c.y = 0;
  v.firstpov = 0;
  b.dirt = 0;

  if(argc > 1){
    pt = argv[1];
    if(std::filesystem::exists(pt)){
      openfile(pt, b);
    }else{
      b.rows.push_back("");
    }
  }else{
    std::cerr << "Usage : gim <filename>\n";
    return 1;
  }


  init();

  define_key("\033[1;5A", KEY_CTRL_UP);
  define_key("\033[1;5B", KEY_CTRL_DOWN);
  define_key("\033[1;5C", KEY_CTRL_RIGHT);
  define_key("\033[1;5D", KEY_CTRL_LEFT);

  printfile(v, b);

  WINDOW *status = newwin(1, COLS, LINES - 1, 0); // statusbar
  if(has_colors()) {
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    wbkgd(status, COLOR_PAIR(1));
  }else{
    std::cerr << "the terminal does not support color\n";
    return 1;
  }


  ref(c, v, status, pt, b);

  while(true){
    ch = getch();
    povupdate = v.firstpov;
    handleinput(status);
    if(povupdate != v.firstpov) 
      printfile(v, b);
    ref(c, v, status, pt, b);
  }

  endwin();
  return 0;
}
