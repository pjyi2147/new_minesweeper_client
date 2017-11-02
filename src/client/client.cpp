#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include "../common/json.hpp"
#include "../common/minesweeper.h"
#include "client.h"

using namespace std;
using boost::asio::ip::tcp;
using json = nlohmann::json;

void Setup(MineSweeper* m, json from_server_json) {
  int m_col = from_server_json["col"];
  int m_row = from_server_json["row"];
  int m_mine_num = from_server_json["mine_num"];

  MineSweeper m_copy(m_col, m_row, m_mine_num);
  
  Update(&m_copy, from_server_json);

  m = new MineSweeper(m_copy);
}

void Update(MineSweeper* m, json from_server_json) {
  int m_col = m->getCol();
  int m_row = m->getRow();

  if (m_col != from_server_json["col"] ||
      m_row != from_server_json["row"] ||
      m->getMineNum() != from_server_json["mine_num"]) {
    
    Setup(m, from_server_json);

    return;
  }

  m->setGameEnd(from_server_json["game_end"]);
  m->setWin(from_server_json["win"]);
  string mine_state = from_server_json["minefield"];

  int length_mine_state = mine_state.length();
  for (int i = 0; i < length_mine_state; ++i)
  {
    int tile_row = i / m_col;
    int tile_col = i % m_col;
    // when numbers
    int k = mine_state[i];
    if (isdigit(k))
    {
      m->setNeighborCountTile(tile_col, tile_row, k - 48);
      m->setRevealedTile(tile_col, tile_row, true);
      if (m->getNeighborCountTile(tile_col, tile_row) == 0)
      {
        m->setDoneTile(tile_col, tile_row, true);
      }
    }
    // when flagged;
    else if (k == 'F')
    {
      m->setFlagTile(tile_col, tile_row, true);
    }
    // when covered;
    else if (k == 'C')
    {
      m->setRevealedTile(tile_col, tile_row, false);
    }
    // when mined;
    else if (k == 'M')
    {
      m->setRevealedTile(tile_col, tile_row, true);
      m->setMineTile(tile_col, tile_row, true);
    }
  }
}

void Transfer(MineSweeper* m, json to_server_json) {
  try
  {
    // setup
    boost::asio::io_service io_service;
    tcp::resolver resolver(io_service);
    tcp::resolver::query query("localhost", "1234");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
    tcp::socket socket(io_service);

    boost::asio::connect(socket, endpoint_iterator);

    // write
    boost::system::error_code ignored_error;

    string message = to_server_json.dump();
    boost::asio::write(socket, boost::asio::buffer(message), ignored_error);
    cout << "Message send" << endl;

    // read
    cout << "Reading messages..." << endl;
    boost::array<char, 700> buf;
    boost::system::error_code error;
    size_t len = socket.read_some(boost::asio::buffer(buf), error);

    if (error)
      throw boost::system::system_error(error);
    auto message_rec = string(buf.begin(), buf.begin() + len);
    cout << "Message read: " << message_rec << endl;
    json from_server_json = json::parse(message_rec);
    
    Update(m, from_server_json);
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
}