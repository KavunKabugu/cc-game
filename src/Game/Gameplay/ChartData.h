#ifndef CC_GAME_CHART_DATA_H
#define CC_GAME_CHART_DATA_H

#include <vector>

namespace Game::Gameplay {

struct ChartNote {
    double hitTime; // Seconds, includes the chart's offset.
    int lane;       // 0..3
};

struct ChartData {
    int formatVersion = 0;
    double offsetSeconds = 0.0;
    std::vector<ChartNote> notes; // Sorted ascending by hitTime.
};

} // namespace Game::Gameplay

#endif // CC_GAME_CHART_DATA_H
