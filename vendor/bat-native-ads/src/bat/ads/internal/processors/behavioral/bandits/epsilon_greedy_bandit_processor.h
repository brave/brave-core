/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_

#include <string>

namespace ads::processor {

struct BanditFeedbackInfo;

class EpsilonGreedyBandit final {
 public:
  EpsilonGreedyBandit();

  EpsilonGreedyBandit(const EpsilonGreedyBandit&) = delete;
  EpsilonGreedyBandit& operator=(const EpsilonGreedyBandit&) = delete;

  ~EpsilonGreedyBandit() = default;

  void Process(const BanditFeedbackInfo& feedback);

 private:
  void InitializeArms() const;

  void UpdateArm(int reward, const std::string& segment) const;
};

}  // namespace ads::processor

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_
