/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/behavioral/bandit_classifier/bandit_classifier.h"

#include <stdint.h>

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/rand_util.h"
#include "bat/ads/internal/ad_targeting/ad_targeting.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_targeting {
namespace behavioral {

namespace {
const int kTopWinningCategoryCount = 3;

// TODO(Moritz Haller): Use alias
struct Arm {
  std::string name;
  double value;
  uint64_t pulls;
}

// TODO(Moritz Haller): Init/read/write from DB
// serialize/deser in here >> persist as string pref
std::vector<Arm> kArms = {
  {"architecture", 1.0, 0},
  {"arts & entertainment", 1.0, 0},
  {"automotive", 1.0, 0},
  {"business", 1.0, 0},
  {"careers", 1.0, 0},
  {"cell phones", 1.0, 0},
  {"drugs", 1.0, 0},
  {"education", 1.0, 0},
  {"family & parenting", 1.0, 0},
  {"fashion", 1.0, 0},
  {"folklore", 1.0, 0},
  {"food & drink", 1.0, 0},
  {"health & fitness", 1.0, 0},
  {"history", 1.0, 0},
  {"hobbies & interests", 1.0, 0},
  {"home", 1.0, 0},
  {"law", 1.0, 0},
  {"military", 1.0, 0},
  {"personal finance", 1.0, 0},
  {"pets", 1.0, 0},
  {"politics", 1.0, 0},
  {"real estate", 1.0, 0},
  {"religion", 1.0, 0},
  {"science", 1.0, 0},
  {"society", 1.0, 0},
  {"sports", 1.0, 0},
  {"technology & computing", 1.0, 0},
  {"travel", 1.0, 0},
  {"weather", 1.0, 0},
  {"crypto", 1.0, 0},
};

}  // namespace

BanditClassifier::BanditClassifier(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

BanditClassifier::~BanditClassifier() = default;

// SERVING SIDE
// keep public: communicate learned estimates to client
CategoryList BanditClassifier::GetWinningCategories() const {
  CategoryList winning_categories = ChooseArms(kArms,
      kTopWinningCategoryCount, kEpsilon);  // TODO(Moritz Haller): inject epsilon NOLINT

  // TODO(Moritz Haller): Remove
  for (const auto& segment : winning_categories) {
    BLOG(1, "*** " << segment);
  }

  return winning_categories;
}


// MODEL: internal representation
// - observer or DB?
//   - db might be purged?
//   - 
// keep public: "sensor 1"
void BanditClassifier::RegisterAction(
    std::string arm_name) {
  // TODO(Moritz Haller): +1 to pulls for arm with name == arm_name
  // should always be incremented before feedback
}

// keep public: "sensor 2"
void BanditClassifier::RegisterFeedback(
    std::string arm_name) {
  UpdateValueEstimates(arm_name)
}

// eps - param
// estimates per arm - persist
// pulls per arm - persist

// TODO(Moritz Haller): make "actuator" private?
std::map<std::string, double> BanditClassifier::ChooseArms(
    std::map<std::string, double> segments,  // TODO(Moritz Haller): pass by value since we shuffle in place NOLINT
    uint_16 arms,
    double epsilon) {
  // Explore
  if (base::RandDouble() < epsilon) {
      // Sample without replacement
      base::RandomShuffle(begin(segments), end(segments));
      return winning_categories(segments.begin(), segments.begin() + arms);
  }

  // Exploit
  std::vector<std::pair<std::string, double>> top_segments(
      kTopWinningCategoryCount);
  std::partial_sort_copy(segments.begin(), segments.end(),
      top_segments.begin(), top_segments.end(), [](
    std::pair<const std::string, double> const& l,
    std::pair<const std::string, double> const& r) {
      if (l.second == r.second) {
        // TOOD(Moritz Haller): Break ties randomly
        // Find different impl since using LSB might lack entropy
        const int rand = 0;
        return (rand & 1) == 1;
      }
    return l.second > r.second;
  });

  std::map<std::string, double> winning_categories;
  for (const auto& pair in top_segments) {
    winning_categoris.insert(pair)
  }
  return winning_categories;
}

// private
void BanditClassifier::UpdateValueEstimates(
    std::string arm_name) {
  // self.estimates[arm] = self.estimates[arm] + 1 / self.pulls[arm] * (reward - self.estimates[arm])
  return;
}

}  // namespace behavioral
}  // namespace ad_targeting
}  // namespace ads
