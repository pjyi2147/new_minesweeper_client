#pragma once

void Setup(MineSweeper* m, nlohmann::json from_server_json);
void Update(MineSweeper* m, nlohmann::json);
void Transfer(MineSweeper* m, nlohmann::json to_server_json);