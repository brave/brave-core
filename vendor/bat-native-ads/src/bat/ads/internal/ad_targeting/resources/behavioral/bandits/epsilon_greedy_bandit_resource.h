/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_  // NOLINT

#include "bat/ads/internal/ad_targeting/resources/resource.h"

#include "bat/ads/internal/ad_targeting/ad_targeting_segment.h"

namespace ads {
namespace ad_targeting {
namespace resource {

class EpsilonGreedyBandit : public Resource<SegmentList> {
 public:
  EpsilonGreedyBandit();

  ~EpsilonGreedyBandit() override;

  bool IsInitialized() const override;

  void LoadFromDatabase();

  SegmentList get() const override;

 private:
  bool is_initialized_ = false;
};

}  // namespace resource
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_  // NOLINT
