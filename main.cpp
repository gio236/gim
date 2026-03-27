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

#include "struct/Cursorstruct.hpp"
#include "struct/Viewportstruct.hpp"
#include "struct/Bufferstruct.hpp"
#include "def/def.hpp"
#include "syntax_highlighter.h"

#ifndef TABSPACE
#define TABSPACE 2
#endif

std::string statusmessage = DEFMESS;
SyntaxHighlighter highlighter;

// Calcola la dimensione del file
int byteslen(const Buffer &b){
  int bytes = b.rows.size(); 
  for(const auto& r : b.rows) bytes += r.length();
  return bytes;
}

// Calcola la colonna visiva con Tab a 2
int calcvcol(const std::string &row, int x){
  int vcol = 0;
  for(int i = 0; i < x; i++){
    if(row[i] == '\t') vcol += TABSPACE - (vcol % TABSPACE);
    else vcol++;
  }
  return vcol;
}

// Salva il buffer su disco
void savefile(Buffer &b, const std::string &pt){
  std::ofstream file(pt);
  if(file.is_open()){
    for(const auto& r : b.rows) file << r << std::endl;
    file.close();
    b.resetvalue();
  }
}

// Carica il file nel buffer
void openfile(const std::string &filename, Buffer &b){
  std::ifstream file(filename);
  if(file.is_open()){
    std::string line;
    while(getline(file, line)) b.rows.push_back(line);
    if(b.rows.empty()) b.rows.push_back("");
    file.close();
  } else {
    b.rows.push_back("");
  }
}

void desiredcols(Cursor &c, const Buffer &b, const Viewport &v){
  int len = b.rows[c.y].length();
  if(c.x > len) c.x = len;
  c.mx = calcvcol(b.rows[c.y], c.x) - v.orizoff;
}

void setmessage(const Cursor &c, WINDOW *status, const std::string &pt, const Buffer &b){
  werase(status);
  std::string mod = b.dirt ? " (modificato)" : "";
  mvwprintw(status, 0, 0, "%s - %d/%d righe%s", pt.c_str(), c.y + 1, (int)b.rows.size(), mod.c_str());
  mvwprintw(status, 0, COLS - 1 - statusmessage.length(), "%s", statusmessage.c_str());
  statusmessage = DEFMESS;
}

void ref(const Cursor &c, const Viewport &v, WINDOW *status, const std::string &pt, const Buffer &b){
  setmessage(c, status, pt, b);
  refresh();
  wrefresh(status);
  move((c.y - v.firstpov), c.mx);
}

void initall(){
  initscr(); noecho(); keypad(stdscr, TRUE); cbreak();
  start_color(); use_default_colors(); 
  termios t; tcgetattr(STDIN_FILENO, &t);
  t.c_iflag &= ~(IXON); tcsetattr(STDIN_FILENO, TCSANOW, &t);
  signal(SIGINT, SIG_IGN); signal(SIGTSTP, SIG_IGN);
}

void handleinput(Cursor &c, Buffer &b, Viewport &v, WINDOW *status, const std::string &pt, int ch){
  switch(ch){
    case '\t':
      b.rows[c.y].insert(c.x, 1, '\t');
      c.x++; b.changestatus();
      break;
    case KEY_BACKSPACE: case 127: case 8:
      if(c.x > 0){
        c.x--; b.rows[c.y].erase(c.x, 1);
      } else if(c.y > 0){
        c.x = b.rows[c.y-1].length();
        b.rows[c.y-1] += b.rows[c.y];
        b.rows.erase(b.rows.begin() + c.y);
        c.y--; if(c.y < v.firstpov) v.firstpov--;
      }
      b.changestatus();
      break;
    case KEY_UP: if(c.y > 0){ c.y--; if(c.y < v.firstpov) v.firstpov--; } break;
    case KEY_DOWN: if(c.y+1 < (int)b.rows.size()){ c.y++; if(c.y-v.firstpov >= LINES-1) v.firstpov++; } break;
    case KEY_LEFT: if(c.x > 0) c.x--; break;
    case KEY_RIGHT: if(c.x < (int)b.rows[c.y].length()) c.x++; break;
    case '\n':
      b.rows.insert(b.rows.begin() + c.y + 1, b.rows[c.y].substr(c.x));
      b.rows[c.y] = b.rows[c.y].substr(0, c.x);
      c.y++; c.x = 0; if(c.y-v.firstpov >= LINES-1) v.firstpov++;
      b.changestatus();
      break;
    case SAVE_KEY:
      savefile(b, pt);
      statusmessage = std::to_string(byteslen(b)) + " byte salvati";
      break;
    case QUIT_KEY:
      if(b.dirt && b.time > 0){
        statusmessage = "Modifiche non salvate! Ctrl-X ancora " + std::to_string(b.time) + " volte";
        b.time--;
      } else { endwin(); exit(0); }
      break;
    default:
      if(isprint(ch)){
        b.rows[c.y].insert(c.x, 1, (char)ch);
        c.x++; b.changestatus();
      }
  }
  desiredcols(c, b, v);
}

int main(int argc, char *argv[]){
  if(argc < 2) { std::cerr << "Uso: gim <file>\n"; return 1; }
  std::string pt = argv[1];
  Buffer b; Cursor c = {0, 0, 0}; Viewport v = {0, 0};

  openfile(pt, b);
  initall();

  if(has_colors()){
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    highlighter.initColors();
    highlighter.setLanguage(highlighter.detectLanguage(pt));
  }

  WINDOW *status = newwin(1, COLS, LINES - 1, 0);
  wbkgd(status, COLOR_PAIR(1));

  while(true){
    clear();
    for(int i = 0; i < LINES - 1 && (i + v.firstpov) < (int)b.rows.size(); i++){
       highlighter.renderLineWithHighlight(i, b.rows[i + v.firstpov], v.orizoff, COLS);
    }
    ref(c, v, status, pt, b);
    handleinput(c, b, v, status, pt, getch());
  }
  return 0;
}
