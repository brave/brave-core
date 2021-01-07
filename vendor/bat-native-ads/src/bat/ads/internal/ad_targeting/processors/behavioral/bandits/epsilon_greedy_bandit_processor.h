/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_  // NOLINT

#include <stdint.h>

#include <string>

#include "bat/ads/internal/ad_targeting/data_types/behavioral/bandits/epsilon_greedy_bandit_arms.h"
#include "bat/ads/internal/ad_targeting/processors/behavioral/bandits/bandit_feedback_info.h"
#include "bat/ads/internal/ad_targeting/processors/processor.h"
#include "bat/ads/mojom.h"

namespace ads {
namespace ad_targeting {
namespace processor {

class EpsilonGreedyBandit : public Processor<BanditFeedbackInfo> {
 public:
  EpsilonGreedyBandit();

  ~EpsilonGreedyBandit() override;

  void Process(
      const BanditFeedbackInfo& feedback) override;

 private:
  void InitializeArms() const;

  void UpdateArm(
      const uint64_t reward,
      const std::string& segment) const;
};

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_  // NOLINT
