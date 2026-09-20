//
// Created by ludeo on 9/19/26.
//

#ifndef CC_GAME_DIFFICULTYCALCULATION_H
#define CC_GAME_DIFFICULTYCALCULATION_H
#include <utility>
#include <debug/vector>
#include "ThirdParty/json.hpp"

namespace Game::PerformancePoints {

    class DifficultyCalculation
    {
        public:
            struct Result {
                double rating = 0.0;
                double peak_index = 0.0;
                double consistency = 0.0;
                double length_multiplier = 1.0;
                double hard_seconds = 0.0;

                std::map<std::string, double> components;
                std::vector<std::pair<double, double>> profile;
                std::vector<double> peaks;

                int note_count = 0;
                int group_count = 0;

                double length_s = 0.0;
                double max_strain = 0.0;
                double top_section_s = 0.0;
            };

            struct NoteGroup {
                float time_ms = 0.0;
                std::vector<int> lanes;

                [[nodiscard]] int size() const {
                    return static_cast<int>(lanes.size());
                }
            };

            static Result CalculateDifficulty(const std::string& chartFilePath);
        private:
            struct RawNote {
                float time_ms;
                int lane;
            };

            static bool contains_lane(const std::vector<int>& lanes, int lane);
            static int lane_overlap(const NoteGroup& a, const NoteGroup& b);
            static std::pair<std::vector<NoteGroup>, nlohmann::json> LoadNoteGroupsFromFile(const std::string& path);
            static std::vector<double> anchor_shares(const std::vector<NoteGroup>& groups);

    };

}

#endif //CC_GAME_DIFFICULTYCALCULATION_H
