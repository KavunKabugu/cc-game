//
// Created by ludeo on 9/19/26.
//

#include "DifficultyCalculation.h"

#include <fstream>
#include <optional>
#include <string>

#include "DifficultyConstants.h"
#include "SDL3/SDL_log.h"
#include "ThirdParty/json.hpp"

namespace Game::PerformancePoints
{
    constexpr int LANE_COUNT = 4;

    bool DifficultyCalculation::contains_lane(const std::vector<int>& lanes, const int lane) {
        return std::ranges::find(lanes, lane) != lanes.end();
    }

    int DifficultyCalculation::lane_overlap(const NoteGroup& a, const NoteGroup& b) {
        int result = 0;

        for (const int lane : a.lanes) {
            if (contains_lane(b.lanes, lane)) ++result;
        }

        return result;
    }

    std::pair<std::vector<DifficultyCalculation::NoteGroup>, nlohmann::json> DifficultyCalculation::LoadNoteGroupsFromFile(const std::filesystem::path& path) {
        std::ifstream file(path);

        if (!file) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "PerformancePointsCalculation | Could not open map: %s", path.c_str());
        }

        nlohmann::json data;
        file >> data;

        std::vector<RawNote> notes;

        if (data.contains("notes")) {
            for (const auto& note : data["notes"]) {
                notes.push_back({ .time_ms = note.at("timeMs").get<float>(), .lane = note.at("lane").get<int>() });
            }
        }

        std::ranges::sort(notes, [](const RawNote& a, const RawNote& b) {
            if (a.time_ms != b.time_ms) return a.time_ms < b.time_ms;

            return a.lane < b.lane;
        });

        std::vector<NoteGroup> groups;

        size_t i = 0;

        while (i < notes.size()) {
            float key = std::round(notes[i].time_ms / chord_window_ms);

            size_t j = i + 1;

            while (j < notes.size()) {
                if (std::round(notes[j].time_ms / chord_window_ms) != key) break;

                ++j;
            }

            std::vector<int> lanes;

            for (size_t k = i; k < j; ++k) {
                if (!contains_lane(lanes, notes[k].lane)) lanes.push_back(notes[k].lane);
            }

            std::ranges::sort(lanes);
            groups.push_back({ .time_ms = notes[i].time_ms, .lanes = lanes});
            i = j;
        }

        nlohmann::json song = data.value("song", nlohmann::json::object());

        return {groups, song};
    }

    // ---------------------------------------------------------------------------
    // Anchor calculation
    // ---------------------------------------------------------------------------

    std::vector<double> DifficultyCalculation::anchor_shares(const std::vector<NoteGroup>& groups) {
        std::vector<double> times;
        std::vector counts(LANE_COUNT, 0);
        std::vector<double> shares;

        for (const auto& [time_ms, lanes] : groups) times.push_back(time_ms);

        size_t lo = 0;

        for (size_t i = 0; i < groups.size(); ++i) {
            for (const int lane : groups[i].lanes) ++counts[lane];

            double cutoff = groups[i].time_ms - anchorWindowMs;
            const size_t new_lo = std::ranges::lower_bound(times, cutoff) - times.begin();

            while (lo < new_lo) {
                for (const int lane : groups[lo].lanes) --counts[lane];

                ++lo;
            }

            if (const int total = std::accumulate(counts.begin(), counts.end(), 0); total >= anchorMinNotes) {
                const int busiest = *std::ranges::max_element(counts);

                shares.push_back(static_cast<double>(busiest) / total);
            } else {
                shares.push_back(0.25);
            }
        }

        return shares;
    }

    DifficultyCalculation::Result DifficultyCalculation::CalculateDifficulty(const std::filesystem::path& chartFilePath) {
        auto [groups, song] = LoadNoteGroupsFromFile(chartFilePath);
        if (groups.size() < 2) return {};

        const std::vector<double> shares = anchor_shares(groups);

        double strain = 0.0;

        std::vector<double> peaks;
        std::vector<std::pair<double, double>> profile;

        double section_end = groups.front().time_ms + sectionMs;
        double section_peak = 0.0;
        double max_strain = 0.0;
        double top_time = groups.front().time_ms;

        std::map<std::string, double> totals = {
            {"base", 0.0},
            {"chord", 0.0},
            {"jack", 0.0},
            {"anchor", 0.0},
            {"rhythm", 0.0}
        };

        std::optional<float> prev_delta;

        for (size_t i = 1; i < groups.size(); ++i) {
            const auto& prev = groups[i - 1];
            const auto& cur = groups[i];

            float delta = std::max(cur.time_ms - prev.time_ms, minDeltaMs);

            while (cur.time_ms > section_end) {
                peaks.push_back(section_peak);
                profile.emplace_back(section_end / 1000.0, section_peak);
                section_end += sectionMs;
                section_peak = strain * std::pow(decayPerSecond, sectionMs / 1000.0);
            }

            // Base speed.
            const double base = std::pow(1000.0 / delta,speedExponent);

            // Chords.
            const double chord_mult = 1.0 + chordWeight * (static_cast<float>(cur.size()) - 1);

            // Jacks.
            const int overlap = lane_overlap(prev, cur);
            double jack_mult = 1.0;

            if (overlap > 0) {
                const double urgency = std::max(0.0, 1.0 - delta / jackFailoffMs);
                const double share = static_cast<double>(overlap) / cur.size();
                jack_mult = 1.0 + jackWeight * urgency * share;

                if (cur.size() > 1 && prev.size() > 1) {
                    jack_mult += chordJackWeight * urgency * share;
                }
            }

            // Anchors.
            double excess = std::max(0.0, shares[i] - anchorThreshold) / (1.0 - anchorThreshold);
            double anchor_mult = 1.0 + anchorWeight * excess;

            // Rhythm irregularity.
            double rhythm_mult = 1.0;

            if (prev_delta.has_value()) {
                const double octaves = std::min(std::abs(std::log2(delta / prev_delta.value())), rhythmCap);
                rhythm_mult = 1.0 + rhythmWeight * octaves;
            }

            prev_delta = delta;
            double value = base * chord_mult * jack_mult * anchor_mult * rhythm_mult;

            totals["base"] += base;
            totals["chord"] += base * (chord_mult - 1.0);
            totals["jack"] += base * chord_mult * (jack_mult - 1.0);
            totals["anchor"] += base * chord_mult * jack_mult * (anchor_mult - 1.0);
            totals["rhythm"] += base * chord_mult * jack_mult * anchor_mult * (rhythm_mult - 1.0);

            // Strain decay.
            strain *= std::pow(decayPerSecond, delta / 1000.0);
            strain += value;
            section_peak = std::max(section_peak, strain);

            if (strain > max_strain) {
                max_strain = strain;
                top_time = cur.time_ms;
            }
        }

        peaks.push_back(section_peak);
        profile.emplace_back(section_end / 1000.0, section_peak);

        std::vector<double> ranked;

        for (double p : peaks) {
            if (p > 0.0) ranked.push_back(p);
        }

        if (ranked.empty()) return {};

        std::ranges::sort(ranked, std::greater());

        // Hardest quarter.
        const int top_n = std::max(1, static_cast<int>(std::ceil(peakFraction * static_cast<float>(ranked.size()))));
        const double peak_index = std::accumulate(ranked.begin(), ranked.begin() + top_n, 0.0) / top_n;

        // Consistency.
        const double average = std::accumulate(ranked.begin(), ranked.end(), 0.0) / static_cast<double>(ranked.size());
        const double consistency = average / peak_index;

        // Hard content.
        const double cutoff = peak_index * sustainThreshold;
        int hard_sections = 0;

        for (const double p : peaks) {
            if (p >= cutoff) ++hard_sections;
        }

        const double hard_seconds = hard_sections * (sectionMs / 1000.0);

        // Length multiplier.
        const double steps = std::log2(1.0 + hard_seconds / lengthReferenceS);
        const double length_mult = 1.0 + lengthWeight * consistency * steps;

        // Final rating.
        const double rating = std::pow(peak_index * length_mult, compressionExponent) * scale;

        double grand = 0.0;

        for (const auto& value : totals | std::views::values) grand += value;

        if (grand == 0.0) grand = 1.0;

        std::map<std::string, double> components;

        for (const auto& [name, value] : totals) components[name] = value / grand;

        int note_count = 0;

        for (const auto& group : groups) note_count += group.size();

        return Result {
            .rating = std::round(rating * 100.0) / 100.0,
            .peak_index = peak_index,
            .consistency = consistency,
            .length_multiplier = length_mult,
            .hard_seconds = hard_seconds,
            .components = components,
            .profile = profile,
            .peaks = ranked,
            .note_count = note_count,
            .group_count = static_cast<int>(groups.size()),
            .length_s = (groups.back().time_ms - groups.front().time_ms) / 1000.0,
            .max_strain = max_strain,
            .top_section_s = top_time / 1000.0
        };
    }

}
