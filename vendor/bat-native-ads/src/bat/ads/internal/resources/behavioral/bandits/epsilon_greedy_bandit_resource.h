/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_

#include "bat/ads/internal/resources/resource_interface.h"
#include "bat/ads/internal/segments/segments_aliases.h"

namespace ads {

struct CatalogInfo;

namespace resource {

class EpsilonGreedyBandit final : public ResourceInterface<SegmentList> {
 public:
  EpsilonGreedyBandit();
  ~EpsilonGreedyBandit() override;
  EpsilonGreedyBandit(const EpsilonGreedyBandit&) = delete;
  EpsilonGreedyBandit& operator=(const EpsilonGreedyBandit&) = delete;

  bool IsInitialized() const override;

  void LoadFromCatalog(const CatalogInfo& catalog);

  SegmentList get() const override;

 private:
  bool is_initialized_ = false;
};

}  // namespace resource
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_H_
