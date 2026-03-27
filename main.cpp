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

/* headers per definizione delle struct */
#include "struct/Cursorstruct.hpp"
#include "struct/Viewportstruct.hpp"
#include "struct/Bufferstruct.hpp"

/* headers per costanti */
#include "def/def.hpp"

/* DEFMESS ==> defualtmessagge definito in def */
std::string statusmessage = DEFMESS;

/* conta quanti bytes occupa il file */
int byteslen(const Buffer &b){
  int bytes = b.rows.size(); 
  for(int i = 0; i < (int)b.rows.size(); i++){
    bytes += b.rows[i].length();
  }
  return bytes;
}

/* calcola la colonna virtuale assoluta nella riga fino a x */
int calcvcol(const std::string &row, int &x){
  int vcol = 0;
  for(int i = 0; i < x; i++){
    if(row[i] == '\t')
      vcol += TABSPACE - (vcol % TABSPACE);
    else
      vcol++;
  }
  return vcol;
}

void desiredcols(Cursor &c, const Buffer &b, const Viewport &v){
  int len = b.rows[c.y].length();
  if(c.x > len) c.x = len;

  int vcol = calcvcol(b.rows[c.y], c.x);
  c.mx = vcol - v.orizoff;
}

void setmessage(const Cursor &c, WINDOW *status, const std::string pt, const Buffer &b){
  werase(status);
  if(b.dirt)
    mvwprintw(status, 0, 0, "%s - %d/%d lines (modified)", pt.c_str(), c.y + 1, (int)b.rows.size());
  else
    mvwprintw(status, 0, 0, "%s - %d/%d lines", pt.c_str(), c.y + 1, (int)b.rows.size());
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

void disableJobControlSignals(){
  signal(SIGINT, SIG_IGN);   /* ignora Ctrl+C */
  signal(SIGTSTP, SIG_IGN);  /* ignora Ctrl+Z */
  signal(SIGQUIT, SIG_IGN);  /* ignora Ctrl+\ */
}

void init(){
  initscr();         
  noecho();         
  keypad(stdscr, TRUE);
  cbreak();
  disableFlowControl();
  idlok(stdscr, TRUE);
  scrollok(stdscr, TRUE);
}

void initall(){
  disableFlowControl();
  disableJobControlSignals();
  init();
}

void savefile(Buffer &b, const std::string &pt){
  std::ofstream file(pt);
  if(file.is_open()){
    for(int i = 0; i < (int)b.rows.size(); i++){
      file << b.rows[i] << std::endl;
    }
    file.close();
    b.resetvalue();
    // b.dirt = 0;
    // b.time = QUITIME;
  }
}

void printrow(const Cursor &c, const Buffer &b, const Viewport &v){
  int row = c.y - v.firstpov;
  move(row, 0);
  clrtoeol();

  int vcol = 0;
  int screen_col = 0;

  for(int i = 0; i < (int)b.rows[c.y].length() && screen_col < COLS - 1; i++){
    if(b.rows[c.y][i] == '\t'){
      int spaces = TABSPACE - (vcol % TABSPACE);
      int vend = vcol + spaces;

      if(vend <= v.orizoff){
        /* tab completamente fuori a sinistra, salta */
        vcol = vend;
        continue;
      }
      /* tab parzialmente o totalmente visibile */
      int vis_start = (vcol < v.orizoff) ? v.orizoff : vcol;
      int vis_spaces = vend - vis_start;
      for(int s = 0; s < vis_spaces && screen_col < COLS - 1; s++)
        mvaddch(row, screen_col++, ' ');
      vcol = vend;
    }else{
      if(vcol >= v.orizoff)
        mvaddch(row, screen_col++, b.rows[c.y][i]);
      vcol++;
    }
  }
}

void orizcontrol(Cursor &c, const Buffer &b, Viewport &v){
  int vcol = calcvcol(b.rows[c.y], c.x);

  if(vcol >= COLS - 1)
    v.orizoff = vcol - COLS + 2;
  else
    v.orizoff = 0;

  c.mx = vcol - v.orizoff;
  printrow(c, b, v);
}

void upmove(Cursor &c, const Buffer &b, Viewport &v){
  if(c.y - 1 >= 0){
    if((c.y - v.firstpov) == 0 && c.y > 0){
      wscrl(stdscr, -1);
      v.firstpov--;
    }
    c.y--;
    desiredcols(c, b, v);
    orizcontrol(c, b, v);
  }
}

void downmove(Cursor &c, const Buffer &b, Viewport &v){
  if(c.y + 1 < (int)b.rows.size()){
    if((c.y - v.firstpov) == LINES - 2){
      wscrl(stdscr, 1);
      v.firstpov++; 
    }
    c.y++;
    desiredcols(c, b, v);
    orizcontrol(c, b, v);
  }
}

void leftmove(Cursor &c, const Buffer &b, Viewport &v){
  if(c.x - 1 >= 0){
    c.x--;
    desiredcols(c, b, v);
    orizcontrol(c, b, v);
  }else if(c.y > 0){
    c.x = b.rows[c.y - 1].length();
    upmove(c, b, v);
  }
}

void rightmove(Cursor &c, const Buffer &b, Viewport &v){
  if(c.x + 1 <= (int)b.rows[c.y].length()){
    c.x++;
    desiredcols(c, b, v);
    orizcontrol(c, b, v);
  }else if(c.y + 1 < (int)b.rows.size()){
    downmove(c, b, v);
    c.x = 0;
    c.mx = 0;
  }
}

void writerow(Cursor &c, Buffer &b, Viewport &v, int ch){
  b.rows[c.y].insert(c.x, 1, char(ch));
  rightmove(c, b, v);

  b.changestatus();
}

bool removechar(Cursor &c, Buffer &b, Viewport &v){
  bool needfullrefresh = false; 

  if(c.x > 0 && c.y >= 0){
    leftmove(c, b, v);
    b.rows[c.y].erase(b.rows[c.y].begin() + c.x);
  }else if(c.y > 0){
    c.x = b.rows[c.y - 1].length();
    b.rows[c.y - 1] += b.rows[c.y];
    b.rows.erase(b.rows.begin() + c.y);
    upmove(c, b, v);
    needfullrefresh = true;
  }else if(b.rows[0].empty() && b.rows.size() > 1){
    b.rows.erase(b.rows.begin() + 0);
    needfullrefresh = true;
  }

  b.changestatus();

  return needfullrefresh;
}

void printfile(const Cursor &c, const Buffer &b, const Viewport &v){
  clear(); 
  int temp = 0;

  for(int i = v.firstpov; i < (int)b.rows.size() && temp < LINES - 1; i++){ 
    if(i == c.y){
      printrow(c, b, v);
    } else {
      /* le righe diverse da c.y non hanno scroll orizzontale */
      int vcol = 0;
      int screen_col = 0;
      for(int j = 0; j < (int)b.rows[i].length() && screen_col < COLS - 1; j++){
        char ch = b.rows[i][j];
        if(ch == '\t'){
          int spaces = TABSPACE - (vcol % TABSPACE);
          for(int s = 0; s < spaces && screen_col < COLS - 1; s++){
            mvaddch(temp, screen_col++, ' ');
          }
          vcol += spaces;
        } else {
          mvaddch(temp, screen_col++, ch);
          vcol++;
        }
      }
    }
    temp++;
  }
  refresh();
}

void insertline(Cursor &c, Buffer &b, Viewport &v){
  b.changestatus();

  b.rows.insert(b.rows.begin() + c.y + 1, b.rows[c.y].substr(c.x));
  b.rows[c.y] = b.rows[c.y].substr(0, c.x);

  downmove(c, b, v);
  c.x = 0;
  c.mx = 0;
  v.orizoff = 0;
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
  b.changestatus();
  b.rows[c.y].insert(c.x, 1, '\t');
  rightmove(c, b, v);
}

void handleinput(Cursor &c, Buffer &b, Viewport &v, WINDOW *status, std::string pt, int ch){
  switch(ch){
    case '\t':
      handletab(c, b, v);
      printrow(c, b, v);
      break;
    case KEY_BACKSPACE: 
      if(removechar(c, b, v)){
        printfile(c, b, v);
      }else{
        printrow(c, b, v);
      }
      break;
    case KEY_LEFT:
      leftmove(c, b, v);
      break;
    case KEY_RIGHT:
      rightmove(c, b, v);
      break;
    case QUIT_KEY:
    {
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
    }
    case KEY_UP:
      upmove(c, b, v);
      break;
    case KEY_DOWN:
      downmove(c, b, v);
      break;
    case '\n':
      insertline(c, b, v);
      printfile(c, b, v);
      break;

    case SAVE_KEY:
    {
      char buffer[100];
      std::snprintf(buffer, sizeof(buffer), "%d bytes written on disk", byteslen(b)); 
      statusmessage.assign(buffer);
      savefile(b, pt);
      break;
    }
    default: 
      if(isprint(ch)){
        writerow(c, b, v, ch);
        printrow(c, b, v);
      }
  }
}

int main(int argc, char *argv[]){


  int ch;
  int povupdate;
  std::string pt;

  Cursor c;
  Buffer b;
  Viewport v;

  c.x = 0;
  c.mx = 0;
  c.y = 0;
  v.firstpov = 0;
  v.orizoff = 0;
  b.dirt = 0;

  if(argc > 1){
    pt = argv[1];
    if(std::filesystem::exists(pt)){
      openfile(pt, b);
    }else{
      b.rows.push_back("");
    }
  }else{
    std::cerr << "Usage: gim <filename>\n";
    return 1;
  }

  initall();

  define_key("\033[1;5A", KEY_CTRL_UP);
  define_key("\033[1;5B", KEY_CTRL_DOWN);
  define_key("\033[1;5C", KEY_CTRL_RIGHT);
  define_key("\033[1;5D", KEY_CTRL_LEFT);

  printfile(c, b, v);

  WINDOW *status = newwin(1, COLS, LINES - 1, 0);
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
    handleinput(c, b, v, status, pt, ch);
    if(povupdate != v.firstpov){
      move(c.y, c.x);
      printrow(c, b, v);
    }
    ref(c, v, status, pt, b);
  }

  endwin();
  return 0;
}
