#ifndef CC_GAME_RESULTS_VIEW_DATA_H
#define CC_GAME_RESULTS_VIEW_DATA_H

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "Game/Gameplay/Judgement.h"
#include "Game/Gameplay/ResultsGraphDisplay.h"

namespace Game::Score {

struct ResultsViewData {
    std::int64_t score{0};
    double accuracyPercent{0.0};
    std::array<int, static_cast<int>(Gameplay::Judgement::Count)> judgementCounts{};
    double biasMs{0.0};
    double stdDevMs{0.0};
    std::string perfectGreatRatioText;

    double chartDomainFirst{0.0};
    double chartDomainLast{0.0};
    std::vector<Gameplay::ResultsGraphEvent> graphEvents;
    std::vector<std::pair<double, double>> accuracySteps;

    std::string songTitle;
    std::string difficultyName;
};

} // namespace Game::Score

#endif // CC_GAME_RESULTS_VIEW_DATA_H
