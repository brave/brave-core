/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_

#include "bat/ads/internal/resources/resource.h"

#include "bat/ads/internal/ad_targeting/ad_targeting_segment.h"

namespace ads {

class Catalog;

namespace resource {

class EpsilonGreedyBandit : public Resource<SegmentList> {
 public:
  EpsilonGreedyBandit();

  ~EpsilonGreedyBandit() override;

  bool IsInitialized() const override;

  void LoadFromCatalog(const Catalog& catalog);

  SegmentList get() const override;

 private:
  bool is_initialized_ = false;
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_
