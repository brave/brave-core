/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/processors/bandits/epsilon_greedy_bandit_processor.h"

#include <algorithm>

#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_targeting {
namespace processor {

namespace {
// const uint16_t kPurchaseIntentDefaultSignalWeight = 1;

// TODO(Moritz Haller): Use alias
struct Arm {
  std::string name;
  double value;
  uint64_t pulls;
};

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

EpsilonGreedyBandit::EpsilonGreedyBandit() = default;

EpsilonGreedyBandit::~EpsilonGreedyBandit() = default;

void EpsilonGreedyBandit::Process(
    const AdNotificationInfo& ad) {
  BLOG(1, "Bandit Process executed");

  // On view +1 to pulls for arm with name == arm_name
  // should always be incremented before feedback

  // On click
  // keep public: "sensor 2"
  // void EpsilonGreedyBandit::RegisterFeedback(
  //     std::string arm_name) {
  //   UpdateValueEstimates(arm_name)
  // }
}

// private
// void EpsilonGreedyBandit::UpdateValueEstimates(
//     std::string arm_name) {
//   // self.estimates[arm] = self.estimates[arm] + 1 / self.pulls[arm] * (reward - self.estimates[arm])
//   return;
// }

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads
