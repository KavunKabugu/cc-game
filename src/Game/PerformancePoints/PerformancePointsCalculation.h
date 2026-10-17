//
// Created by ludeo on 9/18/26.
//

#ifndef CC_GAME_PERFORMANCEPOINTSCALCULATION_H
#define CC_GAME_PERFORMANCEPOINTSCALCULATION_H

#include "Game/Scene/Scenes/ResultsOverlayScene.h"
#include "Game/Score/ResultsViewData.h"
#include "ThirdParty/json.hpp"

namespace Game::PerformancePoints {
    class PerformancePointsCalculation final {
        public:
            PerformancePointsCalculation(const ResultsOverlayContext& resultsOverlayContext, const Score::ResultsViewData& resultsViewData);
            [[nodiscard]] double CalculatePerformancePoints(
                const std::array<int, static_cast<int>(Gameplay::Judgement::Count)>& judgementCounts,
                double accuracyPercent) const;
            void CalculateDifficulty();

        private:
            struct Performance {
                double pp = 0.0;
                double accuracy = 0.0;
                std::optional<double> ratio;

                double accuracy_multiplier = 0.0;
                double ratio_multiplier = 0.0;
                double miss_multiplier = 0.0;
                double bad_multiplier = 0.0;
            };

            struct Judgements {
                int marvelous = 0;
                int perfect = 0;
                int great = 0;
                int good = 0;
                int bad = 0;
                int miss = 0;

                [[nodiscard]] int total() const {
                    return marvelous + perfect + great + good + bad + miss;
                }

                [[nodiscard]] double ratio() const {
                    const int scored = marvelous + perfect;

                    if (scored == 0) return 0;

                    return static_cast<double>(marvelous) / scored;
                }
            };

            ResultsOverlayContext resultsOverlayContext;
            Score::ResultsViewData resultsViewData;
            double difficultyRating = 0.0;

            static Performance CalculatePerformance(double rating, const Judgements& judgements, double accuracy);
    };

}

#endif //CC_GAME_PERFORMANCEPOINTSCALCULATION_H
