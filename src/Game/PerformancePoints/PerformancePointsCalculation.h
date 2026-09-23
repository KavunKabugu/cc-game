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
            [[nodiscard]] double CalculatePerformancePoints(const std::array<int, static_cast<int>(Gameplay::Judgement::Count)>& judgementCounts) const;
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
                int perfect = 0;
                int great = 0;
                int good = 0;
                int bad = 0;
                int miss = 0;

                [[nodiscard]] int total() const {
                    return perfect + great + good + bad + miss;
                }

                [[nodiscard]] double accuracy() const {
                    if (total() == 0) return 0.0;

                    const double numerator = 100.0 * (perfect + great) + 50.0 * good;

                    return numerator / (100.0 * total());
                }

                [[nodiscard]] double ratio() const {
                    const int scored = perfect + great;

                    if (scored == 0) return 0;

                    return static_cast<double>(perfect) / scored;
                }
            };

            ResultsOverlayContext resultsOverlayContext;
            Score::ResultsViewData resultsViewData;
            double difficultyRating = 0.0;

            static Performance CalculatePerformance(double rating, const Judgements& judgements);
    };

}

#endif //CC_GAME_PERFORMANCEPOINTSCALCULATION_H
