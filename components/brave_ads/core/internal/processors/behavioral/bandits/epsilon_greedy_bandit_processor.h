/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_

namespace ads::processor {

struct BanditFeedbackInfo;

class EpsilonGreedyBandit final {
 public:
  EpsilonGreedyBandit();

  static void Process(const BanditFeedbackInfo& feedback);
};

}  // namespace ads::processor

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_
