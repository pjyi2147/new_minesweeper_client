#include <iostream>
#include <boost/thread/thread.hpp>

#include "common/json.hpp"
#include "common/minesweeper.h"
#include "client/ai.h"
#include "client/client.h"

using namespace std;
using json = nlohmann::json;

void GameLoop(MineSweeper* m, json* to_server_json) {
  while(!m->isGameEnd()) {
    // AI(m, to_server_json);
    Transfer(m, *to_server_json);

    m->PrintMineField();
  }
}

int main() {
  int level, m_col, m_row, m_mine_num;
  cout << "MineSweeper AI" << endl << endl;
  cout << "Input level: ";
  cin >> level;
  
  switch (level)
  {
    case 1:
      m_col = 9, m_row = 9, m_mine_num = 10;
      cout << "Beginner Mode" << endl << endl;
      break;
    case 2:
      m_col = 16, m_row = 16, m_mine_num = 40;
      cout << "Intermediate Mode" << endl << endl;
      break;
    case 3:
      m_col = 30, m_row = 16, m_mine_num = 99;
      cout << "Expert Mode" << endl << endl;
      break;
    case 4:
      cout << "Custom Mode" << endl << endl;
      cout << "Input col: ";
      cin >> m_col;
      cout << "Input row: ";
      cin >> m_row;
      cout << "Input mineNum: ";
      cin >> m_mine_num;
      if (m_mine_num > m_col * m_row - 1) 
      {
        cout << "Error: mineNum too big" << endl;
        cout << "Input mineNum: ";

        cin >> m_mine_num;
      }
  }
  json to_server_json; 
  
  MineSweeper minesweeper(m_col, m_row, m_mine_num);

  vector<string> scripts = {"E 0 0"};

  to_server_json["row"] = m_row;
  to_server_json["col"] = m_col;
  to_server_json["mine_num"] = m_mine_num;
  to_server_json["scripts"] = scripts;
  GameLoop(&minesweeper, &to_server_json);
  
  minesweeper.PrintMineField();
  // @TODO: add statistics here
  /*
  if (to_server_json["win"])
  {
    cout << "won!" << endl;
  }
  else
  {
    cout << "lost..." << endl;
  }
  */
  cin.ignore();
  cin.get();
  return 0;
}