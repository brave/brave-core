/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_

#include <cstdint>
#include <string>

#include "bat/ads/internal/ad_targeting/processors/behavioral/bandits/bandit_feedback_info.h"
#include "bat/ads/internal/ad_targeting/processors/processor_interface.h"

namespace ads {
namespace ad_targeting {
namespace processor {

class EpsilonGreedyBandit final
    : public ProcessorInterface<BanditFeedbackInfo> {
 public:
  EpsilonGreedyBandit();
  ~EpsilonGreedyBandit() override;

  void Process(const BanditFeedbackInfo& feedback) override;

 private:
  void InitializeArms() const;

  void UpdateArm(const uint64_t reward, const std::string& segment) const;
};

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_
