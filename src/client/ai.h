#pragma once

string SimpleClick(int col, int row);
string DoubleClick(int col, int row);
string FlagClick(int col, int row);

void BasicFlagging(MineSweeper* m, std::vector<string>* scripts);
void BasicDoubleClicking(MineSweeper* m, std::vector<string>* scripts);

void RandomGuess(MineSweeper*, std::vector<string>* scripts);

std::vector<int> GetBorderTiles(MineSweeper*);
std::vector<std::vector<int>> GetConnectedBorderTiles(MineSweeper*);
std::vector<std::vector<std::vector<int>>> GetGroupedBorderTiles(MineSweeper*);


void AI(MineSweeper* m, nlohmann::json*);
