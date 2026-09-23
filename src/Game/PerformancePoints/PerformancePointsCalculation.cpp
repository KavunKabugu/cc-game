//
// Created by ludeo on 9/18/26.
//

#include "PerformancePointsCalculation.h"

#include <cmath>
#include <fstream>

#include "DifficultyCalculation.h"
#include "PerformancePointsConstants.h"
#include "Game/Scene/Scenes/ResultsOverlayScene.h"
#include "Game/Score/ResultsViewData.h"
#include "Game/Song/SongManager.h"
#include "ThirdParty/json.hpp"

namespace Game::PerformancePoints {

void PerformancePointsCalculation::CalculateDifficulty() {
    auto difficulties = this->resultsOverlayContext.song->difficulties;
    std::filesystem::path chartPath;

    if (this->resultsOverlayContext.difficultyIndex == -1) {
        auto resultViewData = this->resultsViewData;
        auto it = std::ranges::find_if(difficulties, [resultViewData](const Song::SongDifficulty& obj) {return obj.name == resultViewData.difficultyName;});
        chartPath = it->chartPath;

    } else {
        chartPath = difficulties[resultsOverlayContext.difficultyIndex].chartPath;
    }

    const std::filesystem::path chartFilePath = Song::SongManager::ResolveSongFile(*this->resultsOverlayContext.song, chartPath);
    const DifficultyCalculation::Result difficulty = DifficultyCalculation::CalculateDifficulty(chartFilePath);
    this->difficultyRating = difficulty.rating;
}

PerformancePointsCalculation::PerformancePointsCalculation(const ResultsOverlayContext& resultsOverlayContext, const Score::ResultsViewData& resultsViewData) {
    this->resultsViewData = resultsViewData;
    this->resultsOverlayContext = resultsOverlayContext;
}

PerformancePointsCalculation::Performance PerformancePointsCalculation::CalculatePerformance(const double rating, const Judgements& judgements) {
    const double accuracy = judgements.accuracy();
    auto ratio = judgements.ratio();

    const double base = std::pow(std::max(rating, 0.0) / 10.0, difficultyExponent) * baseValueAtDifficultyTen;
    constexpr double span = 1.0 - accuracyFloor;
    const double normalized = (accuracy - accuracyFloor) / span;

    const double acc_mult = std::pow(std::max(0.0, normalized), accuracyExponent);
    const double miss_mult = std::pow(missPenalty, judgements.miss);
    const double bad_mult = std::pow(badPenalty, judgements.bad);
    double ratio_mult = 1.0;

    if (ratio > 0) ratio_mult = 1.0 + ratioWeight * (ratio - ratioBaseline);

    double total = base * acc_mult * ratio_mult * miss_mult * bad_mult;

    if (judgements.miss == 0) total *= fcBonus;

    return Performance {
        .pp = std::round(total * 100.0) / 100.0,
        .accuracy = accuracy,
        .ratio = ratio,
        .accuracy_multiplier = acc_mult,
        .ratio_multiplier = ratio_mult,
        .miss_multiplier = miss_mult,
        .bad_multiplier = bad_mult
    };
}

double PerformancePointsCalculation::CalculatePerformancePoints(const std::array<int, static_cast<int>(Gameplay::Judgement::Count)>& judgementCounts) const {
    const Judgements judgements{
        .perfect = judgementCounts[0],
        .great = judgementCounts[1],
        .good = judgementCounts[2],
        .bad = judgementCounts[3],
        .miss = judgementCounts[4]
    };

    if (judgements.total() > 0) {
        const Performance perf = CalculatePerformance(this->difficultyRating, judgements);

        return perf.pp;
    }

    return 0.0;
}

}