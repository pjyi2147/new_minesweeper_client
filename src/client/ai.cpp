#include <iostream>
#include <vector>
#include <list>
#include <map> 
#include <string>
#include <sstream>
#include <ctime>

#include "../common/minesweeper.h"
#include "../common/json.hpp"
#include "client.h"

using namespace std;
using json = nlohmann::json;

// Click script generator functions
// simple click 
string SimpleClick(int col, int row)
{
  stringstream s;
  s << "E " << to_string(col) << " " << to_string(row);
  return s.str();
}

// double click
string DoubleClick(int col, int row)
{
  stringstream s;
  s << "D " << to_string(col) << " " << to_string(row);
  return s.str();
}

// flag click
string FlagClick(int col, int row)
{
    stringstream s;
    s << "F " << to_string(col) << " " << to_string(row);
    return s.str();
}


void BasicFlagging(MineSweeper* m, vector<string>* scripts) {
  int m_col = m->getCol(), m_row = m->getRow();
  for (int r = 0; r < m_row; ++r) { 
    for (int c = 0; c < m_col; ++c) {
      // when tile is done or covered, then we dont need to examine
      if (m->isDoneTile(c, r) || !m->isRevealedTile(c, r)) continue;

      if (m->getNeighborCountTile(c, r) == m->CountCoveredTile(c, r)) {
        for (int yoff = -1; yoff <= 1; ++yoff) {
          for (int xoff = -1; xoff <= 1; ++xoff) {
            if (xoff != 0 || yoff != 0) {
              int tile_col = c + xoff;
              int tile_row = r + yoff;

              if (tile_col > -1 && tile_col < m_col
                  && tile_row > -1 && tile_row < m_row) {
                if (!m->isRevealedTile(tile_col, tile_row)
                    && !m->isFlaggedTile(tile_col, tile_row)) {
                  scripts->push_back(FlagClick(tile_col, tile_row));

                  m->setFlagTile(tile_col, tile_row, true);
                  m->setDoneTile(tile_col, tile_row, true);
                  
                  cout << "Flagged col: " << tile_col << " row: "
                       << tile_row << endl;
                }
              }
            }
          }
        }
      }
    }
  }
}

void BasicDoubleClicking(MineSweeper* m, vector<string>* scripts) {
  int m_col = m->getCol(), m_row = m->getRow();
  for (int r = 0; r < m_row; ++r) {
    for (int c = 0; c < m_col; ++c) {
      // only care when the tile is revealed
      if (m->isRevealedTile(c, r) && !m->isDoneTile(c, r)) {
        if (m->getNeighborCountTile(c, r) == m->CountFlagTile(c, r)) {
          scripts->push_back(DoubleClick(c, r));
          m->setDoneTile(c, r, true);
          cout << "Double-Clicked col: " << c << " row: " << r << endl;
        }
      }
    }
  }
}


void RandomGuess(MineSweeper* m, std::vector<string>* scripts) {
  auto empty_tiles = m->getUntouchedTiles();
  int mine_num = m->getMineNum();
  int num_flags = m->CountAllFlagged();

  srand(time(NULL));
  int random_empty_tile = empty_tiles[rand() % empty_tiles.size()];
  int col = random_empty_tile % m->getCol();
  int row = random_empty_tile / m->getCol();

  scripts->push_back(SimpleClick(col, row));
  cout << "AI guessed Col: " << col << " Row: " 
       << row << " from the empty tiles" << endl;
  cout << "with " 
       << double(mine_num - num_flags) / double(empty_tiles.size()) * 100
       << "% chance of the tile being mine." << endl;

}

vector<int> GetBorderTiles(MineSweeper* m) {
  int m_row = m->getRow(), m_col = m->getCol();

  vector<int> border_tiles;

  for (int r = 0; r < m_row; ++r) {
    for (int c = 0; c < m_col; ++c) {
      // when the tile is done or revealed or flagged, then continue;
      if (m->isDoneTile(c, r) ||
          m->isRevealedTile(c, r) || 
          m->isFlaggedTile(c, r)) continue;

      // examine 8 adjacent tiles of the unrevealed tile
      for (int yoff = -1; yoff <= 1; ++yoff) {
        bool border = false;
        for (int xoff = -1; xoff <= 1; ++xoff) {
          if (xoff != 0 || yoff != 0) {
            int t_col = c + xoff;
            int t_row = r + yoff;

            // should not be out of bounds
            if (t_col > -1 && t_col < m_col
                && t_row > -1 && t_row < m_row) {
              // if one of the neighboring tile is revealed;
              if (m->isRevealedTile(t_col, t_row)) {
                // then it is the border tile!
                border_tiles.push_back(r * m_col + c);
                border = true;
                break;
              }
            }
          }
        }
        if (border) break;
      }
    }
  }
  return border_tiles;
}

vector<vector<int>> GetConnectedBorderTiles(MineSweeper* m) {
  int m_row = m->getRow(), m_col = m->getCol();
  auto border_tiles = GetBorderTiles(m);

  vector<vector<int>> connected_border_tiles;

  vector<int> finished_tiles;

  while (true) {
    list<int> queue;
    vector<int> section;

    for (auto& tile: border_tiles) {
      // search the tile is in already in finished_tiles;
      if (find(finished_tiles.begin(), finished_tiles.end(), tile) 
          == finished_tiles.end()) {
        // not found, put it in the queue
        queue.push_back(tile);
        break;
      }
    }

    if (queue.size() == 0) {
      // all sections are finished since there is
      // nothing in queue.
      // therefore, we break
      break;
    }

    while (queue.size() != 0) {
      int current_tile = queue.front();
      queue.pop_front();

      int cur_col = current_tile % m_col; 
      int cur_row = current_tile / m_col;

      section.push_back(current_tile);
      finished_tiles.push_back(current_tile);

      for (auto tile: border_tiles) {
        int t_col = tile % m_col, t_row = tile / m_row; 

        bool is_connected = false;

        if (find(section.begin(), section.end(), tile) != section.end()) {
          // if the tile is found,
          // we do not care about the tile anymore
          continue;
        }

        if (abs(cur_col - t_col) <= 2 && abs(cur_row - t_row) <= 2) {
          for (int r = 0; r < m_row; ++r) { 
            for (int c = 0; c < m_col; ++c) {
              if (m->isRevealedTile(c, r)) {
                if (abs(cur_col - c) <= 1 && abs(cur_row - r) <= 1
                    && abs(t_col - c) <= 1 && abs(t_row - r) <= 1) {
                  is_connected = true;
                  break;
                }
              }
            }
          }
        }

        if (!is_connected) continue;

        // if the connected tile for the section is not in queue
        if (find(queue.begin(), queue.end(), tile) == queue.end()) {
          // add the tile in the queue and check for this tile later
          queue.push_back(tile);
        }
      }
    }
    // one section is done, add the section to the connected_border_tiles
    connected_border_tiles.push_back(section);
  }
  return connected_border_tiles;
}

vector<vector<vector<int>>> GetGroupedBorderTiles(MineSweeper* m) {
  // what are we going to return
  vector<vector<vector<int>>> grouped_border_tiles;    
  
  int m_col = m->getCol(), m_row = m->getRow();

  // get connected_border_tiles
  auto connected_border_tiles = GetConnectedBorderTiles(m);

  for (auto& section : connected_border_tiles) {
    // a grouped section will be stored here
    vector<vector<int>> grouped_section;
    
    while (section.size() > 0) {
      // when there are tiles in the section
      int first_tile = section.front();
      int f_col = first_tile % m_col, f_row = first_tile / m_col;
      section.erase(section.begin());

      vector<int> grouped_tiles;

      auto f_tile_revealed_nghbr_list = m->getRevealedNeighborsTile(f_col, f_row);

      grouped_tiles.push_back(first_tile);
      int i = 0;
      while (i < section.size()) {
        int tile = section[i];
        int t_col = tile % m_col, t_row = tile / m_col; 

        if (m->getRevealedNeighborsTile(t_col, t_row) 
            == f_tile_revealed_nghbr_list) {
          
          grouped_tiles.push_back(tile);
          section.erase(section.begin() + i);
        }
        else ++i;
      }
      grouped_section.push_back(grouped_tiles);
    }
    grouped_border_tiles.push_back(grouped_section);
  }
  return grouped_border_tiles;
}


void AI(MineSweeper* m, json* to_server_json) {
  vector<string> scripts;

  BasicFlagging(m, &scripts);
  BasicDoubleClicking(m, &scripts);

  if (scripts.size() == 0) {
    cout << "Guesses start..." << endl;
    RandomGuess(m, &scripts);
    // @TODO: Implement BruteSolver
  }

  (*to_server_json)["scripts"] = scripts;
}
