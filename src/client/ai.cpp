#include <iostream>
#include <vector>
#include <list>
#include <map> 
#include <string>
#include <sstream>
#include <ctime>
#include <math.h>

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

unsigned NCK( unsigned n, unsigned k )
{
    if (k > n) return 0;
    if (k * 2 > n) k = n-k;
    if (k == 0) return 1;

    int result = n;
    for( int i = 2; i <= k; ++i ) {
        result *= (n-i+1);
        result /= i;
    }
    return result;
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
  
  if (empty_tiles.empty()) {
    cout << "There is no tile to guess. Skipping..." << endl;
    return;
  }

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

      for (auto& tile: border_tiles) {
        int t_col = tile % m_col, t_row = tile / m_col; 

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
                }
              }
              if (is_connected) break;
            }
            if (is_connected) break;
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

  for (auto section : connected_border_tiles) {
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













void GroupedSectionSolFinder(MineSweeper* m, vector<vector<int>> section,
  vector<vector<int>>* section_sol, vector<int>* group_sol, int depth) {

  int m_col = m->getCol(), m_row = m->getRow();

  // excaper
  if (depth == section.size()) {
    for (auto& group : section) {
      int tile = group[0];
      int t_row = tile / m_col, t_col = tile % m_col;

      auto revealed_tiles = m->getRevealedNeighborsTile(t_col, t_row);

      for (auto& r_tile : revealed_tiles) {
        int r_t_row = r_tile / m_col, r_t_col = r_tile % m_col;

        if (m->CountFlagTile(r_t_col, r_t_row) 
            != m->getNeighborCountTile(r_t_col, r_t_row)) {
          return;
        }
      }
    }
    // if we dont have the function returned
    // then we have a solution.
    section_sol->push_back(*group_sol);
  }
  // recurser
  else {
    // check there are more flags than it supposed to
    // we treat one group as one tile 
    // so it is sufficient to just check the first tile
    for (auto& group : section) {
      int tile = group[0];
      int t_row = tile / m_col, t_col = tile % m_col;

      auto revealed_tiles = m->getRevealedNeighborsTile(t_col, t_row);
      for (auto& r_tile : revealed_tiles)  {
        int r_t_col = r_tile % m_col, r_t_row = r_tile / m_col;
        int count_f = m->CountFlagTile(r_t_col, r_t_row);
        int count_n = m->getNeighborCountTile(r_t_col, r_t_row);
        
        if (count_f > count_n) {
          return;
        }
      }
    }

    auto group_1 = section[depth];

    for (int mine_count = 0; mine_count < group_1.size() + 1; ++mine_count) {
      if (mine_count == 0) {
        group_sol->push_back(mine_count);
        GroupedSectionSolFinder(m, section, section_sol, 
                                group_sol, depth + 1);
        
        group_sol->pop_back();
      }
      else {
        int tile_1 = group_1[mine_count - 1];
        int t_1_row = tile_1 / m_col, t_1_col = tile_1 % m_col;

        m->setFlagTile(t_1_col, t_1_row, true);

        group_sol->push_back(mine_count);
        
        GroupedSectionSolFinder(m, section, section_sol, 
                                group_sol, depth + 1);
        
        group_sol->pop_back();

        // when finished, we have to clean it up
        // flag triggered
        if (mine_count == group_1.size()) {
          for (int k = 0; k < group_1.size(); ++k) {
            int tilek = group_1[k];
            int t_k_row = tilek / m_col, t_k_col = tilek % m_col;
            m->setFlagTile(t_k_col, t_k_row, false);
          }
        }
      }
    }
  }
}

vector<vector<vector<int>>> GroupSolFinder(MineSweeper* m) {

  vector<vector<vector<int>>> grouped_total_sol;
  auto grouped_tiles = GetGroupedBorderTiles(m);
  MineSweeper m_copy(*m);

  for (auto& section : grouped_tiles) {

    cout << "section size: " << section.size() << endl;

    if (section.size() > 30) {
      cout << "  section size is bigger than 30." << endl
          << "  skipping..." << endl;
    }
    else {
      vector<vector<int>> section_sol;
      vector<int> group_sol;
      GroupedSectionSolFinder(&m_copy, section, &section_sol, &group_sol, 0);

      grouped_total_sol.push_back(section_sol);
      group_sol.clear();
    }
  }
  return grouped_total_sol;
}

map<int, double> GroupProbCal(MineSweeper* m) {
  auto solution = GroupSolFinder(m);
  auto grouped_tiles = GetGroupedBorderTiles(m); 
  double random_prob = double(m->getMineNum() - m->CountAllFlagged())
                        / double(m->getUntouchedTiles().size());
  int m_col = m->getCol(), m_row = m->getRow();

  map<int, double> prob_store;
  
  for (int section_c = 0; section_c < solution.size(); ++section_c) {
    auto cur_section_sol = solution[section_c];

    vector<double> group_prob;
    double group_prob_sum = 0;

    for (int sol_c = 0; sol_c < cur_section_sol.size(); ++sol_c) {
      auto one_section_sol = cur_section_sol[sol_c];
      double cur_section_prob = 1;

      for (int group_c = 0; group_c < one_section_sol.size(); ++group_c) {
        auto cur_group_sol = one_section_sol[group_c];
        int cur_group_size = grouped_tiles[section_c][group_c].size();

        cur_section_prob *= NCK(cur_group_size, cur_group_sol);

        int pmp = 0;
        while (pmp < cur_group_size)
        {
          if (pmp < cur_group_sol) cur_section_prob *= random_prob;
          else cur_section_prob *= double(1 - random_prob);
          ++pmp;
        }
      }
      // @TODO:
      // cur_section is 0 since there too many 
      // multiplication of random_prob
      // use log!!! 
      group_prob.push_back(cur_section_prob);
      group_prob_sum += cur_section_prob;
    }

    for (int sol_c = 0; sol_c < cur_section_sol.size(); ++sol_c) {
      auto one_section_sol = cur_section_sol[sol_c];

      for (int group_c = 0; group_c < one_section_sol.size(); ++group_c) {
        auto cur_group_sol = one_section_sol[group_c];
        int cur_group_size = grouped_tiles[section_c][group_c].size();

        if (cur_group_sol == 0) {
          for (int tile : grouped_tiles[section_c][group_c]) {
            auto tile_iterator = prob_store.find(tile);
            if (tile_iterator == prob_store.end()) {
              prob_store[tile] = 0;
            }
          }
        }
        else {
          for (int tile : grouped_tiles[section_c][group_c]) {
            auto tile_iterator = prob_store.find(tile);
            if (tile_iterator == prob_store.end()) {
              prob_store[tile] = group_prob[sol_c] * double(cur_group_sol)
                                  / double(cur_group_size) 
                                  / group_prob_sum;
            }
            else {
              prob_store[tile] += group_prob[sol_c] * double(cur_group_sol)
                                  / double(cur_group_size) 
                                  / group_prob_sum;
            }
          }
        }
      }
    }
  }
  return prob_store;
}


void GroupSolver(MineSweeper* m, vector<string>* scripts) {
  int m_col = m->getCol(), m_row = m->getRow();
  auto prob_store = GroupProbCal(m);
  double random_prob = double(m->getMineNum() - m->CountAllFlagged())
                        / double(m->getUntouchedTiles().size());
  
  auto min_val = min_element(prob_store.begin(), prob_store.end(),
                      [](auto l, auto r) -> bool { return l.second < r.second; });


  if (min_val->second == 0) {
    for (auto p : prob_store) {
      if (p.second == 0) {
        int t_col = p.first % m_col, t_row = p.first / m_col;
    
        std::cout << "AI chosed Col: " << t_col << " Row: "
                        << t_row << " with the chance of "
                        << p.second * 100 << "%"
                        << " the tile being mine" << std::endl;
        scripts->push_back(SimpleClick(t_col, t_row));
      }
    }
  }
  else if (min_val->second > random_prob) {
    cout << "AI guesses a random tile since all the probability is"
        << "bigger than random_prob" << endl;
    RandomGuess(m, scripts);
  }
  else {
    int t_col = min_val->first % m_col, t_row = min_val->first / m_col;
    
    std::cout << "AI chosed Col: " << t_col << " Row: "
              << t_row << " with the chance of "
              << min_val->second * 100 << "%"
              << " the tile being mine" << std::endl;
    scripts->push_back(SimpleClick(t_col, t_row));
  }
}





void AI(MineSweeper* m, json* to_server_json) {
  vector<string> scripts;

  BasicFlagging(m, &scripts);
  BasicDoubleClicking(m, &scripts);

  if (scripts.size() == 0) {
    GroupSolver(m, &scripts);
  }

  (*to_server_json)["scripts"] = scripts;
}
