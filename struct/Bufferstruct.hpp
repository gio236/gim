#pragma once
#include <iostream>
#include <vector>
#include "../def/def.hpp"

struct Buffer{
  std::vector<std::string> rows;
  int dirt;
  int time = QUITIME;

  void resetvalue(void){
    dirt = 0; 
    time = QUITIME;
  }

  /* verra chiamata quando si modifica il file */
  /* perche vogliamo resettare le chiamate al tasto di uscita */
  /* ed incrementare le righe sporche ==> "modificate" */
  void changestatus(void){
    dirt++;
    time = QUITIME;
  }

};
