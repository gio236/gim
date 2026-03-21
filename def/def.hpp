#pragma once
#include <iostream>
#include <string>
#include <ncurses.h>

const int SAVE_KEY = 23; /* ctrl+w */
const int QUIT_KEY = 24; /* ctrl+x */

/* definizioni per ncurses || non toccare */
const int KEY_CTRL_LEFT = 1000; 
const int KEY_CTRL_RIGHT = 1001;
const int KEY_CTRL_UP = 1002; 
const int KEY_CTRL_DOWN = 1003; 

/* esteticamente quanti spazi rappresenta un tab*/
const int  TABSPACE = 2;
/* stringa di default per la bottom bar */
const std::string DEFMESS = "ctrl-w for writing, ctrl-x for exit";
/* quanti volte premere il tasto quit con unsaved changes */
const int QUITIME = 2;
