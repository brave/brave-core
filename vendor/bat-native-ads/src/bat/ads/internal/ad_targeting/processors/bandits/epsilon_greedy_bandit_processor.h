/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_  // NOLINT

#include <map>
#include <string>

#include "bat/ads/internal/ad_targeting/processors/processor.h"
#include "brave/components/brave_ads/browser/ad_notification.h"


namespace ads {
namespace ad_targeting {
namespace processor {

class EpsilonGreedyBandit : public Processor<AdNotificationInfo> {
 public:
  EpsilonGreedyBandit();

  ~EpsilonGreedyBandit() override;

  void Process(
      const AdNotificationInfo& ad) override;
};

}  // namespace processor
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_PROCESSORS_BANDITS_EPSILON_GREEDY_BANDIT_PROCESSOR_H_  // NOLINT
