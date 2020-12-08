/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/bandits/epsilon_greedy_bandit.h"

#include <stdint.h>

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/rand_util.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_targeting {
namespace model {

namespace {
// const size_t kMaximumSegments = 3;
}  // namespace

EpsilonGreedyBandit::EpsilonGreedyBandit() = default;

EpsilonGreedyBandit::~EpsilonGreedyBandit() = default;

SegmentList EpsilonGreedyBandit::GetSegments() const {
  // CategoryList winning_categories = ChooseArms(kArms,
  //     kTopWinningCategoryCount, kEpsilon);  // TODO(Moritz Haller): inject epsilon NOLINT
  SegmentList segments;
  return segments;
}


// TODO(Moritz Haller): make "actuator" private?
// std::map<std::string, double> EpsilonGreedyBandit::ChooseArms(
//     std::map<std::string, double> segments,  // TODO(Moritz Haller): pass by value since we shuffle in place NOLINT
//     uint_16 arms,
//     double epsilon) {
//   // Explore
//   if (base::RandDouble() < epsilon) {
//       // Sample without replacement
//       base::RandomShuffle(begin(segments), end(segments));
//       return winning_categories(segments.begin(), segments.begin() + arms);
//   }

//   // Exploit
//   std::vector<std::pair<std::string, double>> top_segments(
//       kTopWinningCategoryCount);
//   std::partial_sort_copy(segments.begin(), segments.end(),
//       top_segments.begin(), top_segments.end(), [](
//     std::pair<const std::string, double> const& l,
//     std::pair<const std::string, double> const& r) {
//       if (l.second == r.second) {
//         // TOOD(Moritz Haller): Break ties randomly
//         // Find different impl since using LSB might lack entropy
//         const int rand = 0;
//         return (rand & 1) == 1;
//       }
//     return l.second > r.second;
//   });

//   std::map<std::string, double> winning_categories;
//   for (const auto& pair in top_segments) {
//     winning_categoris.insert(pair)
//   }
//   return winning_categories;
// }

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads
