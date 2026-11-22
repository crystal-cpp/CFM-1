#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <cctype>
#include "mainplayer.h"

class PlayerDataBase {
public:
    bool LoadFromCSV(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Cannot open file (database): " << filepath << std::endl;
            return false;
        }
        const auto parse_csv_row = [](const std::string& row) {
            std::vector<std::string> fields;
            std::string value;
            bool quoted = false;
            for (size_t i = 0; i < row.size(); ++i) {
                const char ch = row[i];
                if (ch == '"') {
                    if (quoted && i + 1 < row.size() && row[i + 1] == '"') { value += '"'; ++i; }
                    else quoted = !quoted;
                } else if (ch == ',' && !quoted) { fields.push_back(value); value.clear(); }
                else value += ch;
            }
            fields.push_back(value);
            for (std::string& field : fields) {
                const size_t first = field.find_first_not_of(" \t\r\n");
                const size_t last = field.find_last_not_of(" \t\r\n");
                field = first == std::string::npos ? std::string() : field.substr(first, last - first + 1);
            }
            return fields;
        };
        const auto normalize = [](std::string value) {
            std::string out;
            for (unsigned char ch : value) if (ch != ' ' && ch != '_' && ch != '-') out += static_cast<char>(std::tolower(ch));
            return out;
        };
        const auto to_int = [](const std::vector<std::string>& row, int index, int fallback) {
            if (index < 0 || index >= static_cast<int>(row.size()) || row[index].empty()) return fallback;
            try {
                size_t parsed = 0;
                int value = std::stoi(row[index], &parsed);
                return parsed == row[index].size() ? value : fallback;
            } catch (...) { return fallback; }
        };

        std::string header_line;
        if (!std::getline(file, header_line)) return false;
        if (!header_line.empty() && header_line.back() == '\r') header_line.pop_back();
        const std::vector<std::string> headers = parse_csv_row(header_line);
        std::unordered_map<std::string, int> columns;
        for (int i = 0; i < static_cast<int>(headers.size()); ++i) columns[normalize(headers[i])] = i;
        const auto column = [&columns, &normalize](const std::string& name, int fallback = -1) {
            auto it = columns.find(normalize(name));
            return it == columns.end() ? fallback : it->second;
        };

        const int name_col = column("Name", 0), age_col = column("Age", 1);
        const int debut_col = column("DebutAge", 2), potential_col = column("Potential", 3);
        const int position_col = column("Position", 4), pace_col = column("Pace", 5);
        const int shooting_col = column("Shooting", 6), passing_col = column("Passing", 7);
        const int dribbling_col = column("Dribbling", 8), defending_col = column("Defending", 9);
        const int physical_col = column("Physical", 10);

        players.clear();
        allPlayers.clear();
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            const std::vector<std::string> row = parse_csv_row(line);
            if (name_col < 0 || name_col >= static_cast<int>(row.size()) || row[name_col].empty()) continue;
            const std::string name = row[name_col];
            const std::string pos_text = position_col >= 0 && position_col < static_cast<int>(row.size()) ? row[position_col] : "CM";
            const int age = std::clamp(to_int(row, age_col, 22), 0, 99);
            const int debut_age = std::clamp(to_int(row, debut_col, std::clamp(age - 3, 14, 22)), 14, 40);
            const int potential = std::clamp(to_int(row, potential_col, 75), 1, 99);
            Position pos = StringToPosition(pos_text);
            PlayerAttributes attrs{
                std::clamp(to_int(row, pace_col, 60), 1, 99),
                std::clamp(to_int(row, shooting_col, 60), 1, 99),
                std::clamp(to_int(row, passing_col, 60), 1, 99),
                std::clamp(to_int(row, dribbling_col, 60), 1, 99),
                std::clamp(to_int(row, defending_col, 60), 1, 99),
                std::clamp(to_int(row, physical_col, 60), 1, 99)
            };
            auto player = std::make_shared<Player>(name, pos, attrs, age, potential, debut_age);
            players[name] = player;
            allPlayers.push_back(player);
        }
        return !allPlayers.empty();
    }

    std::vector<std::shared_ptr<Player>> GetAcademyPlayers() const {
        std::vector<std::shared_ptr<Player>> academyList;
        for (const auto& player : allPlayers) {
            if (!player->IsAvailableForTransfer()) {
                academyList.push_back(player);
            }
        }
        return academyList;
    }

    std::vector<std::shared_ptr<Player>> GetMarketAvailablePlayers() const {
        std::vector<std::shared_ptr<Player>> marketList;
        for (const auto& player : allPlayers) {
            if (player->IsAvailableForTransfer()) {
                marketList.push_back(player);
            }
        }
        return marketList;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Player>> players;
    std::vector<std::shared_ptr<Player>> allPlayers;

    Position StringToPosition(const std::string& posStr) {
        if (posStr == "GK") return Position::GK;
        if (posStr == "CB") return Position::CB;
        if (posStr == "LB") return Position::LB;
        if (posStr == "RB") return Position::RB;
        if (posStr == "LWB") return Position::LWB;
        if (posStr == "RWB") return Position::RWB;
        if (posStr == "CM") return Position::CM;
        if (posStr == "CDM") return Position::CDM;
        if (posStr == "CAM") return Position::CAM;
        if (posStr == "LM") return Position::LM;
        if (posStr == "RM") return Position::RM;
        if (posStr == "LW") return Position::LW;
        if (posStr == "RW") return Position::RW;
        if (posStr == "CF") return Position::CF;
        if (posStr == "ST") return Position::ST;
        return Position::UNKNOWN;
    }
};