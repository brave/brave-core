/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_UNITTEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_UNITTEST_HELPER_H_

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_processor.h"

namespace brave_ads::test {

class EpsilonGreedyBanditHelper final {
 public:
  EpsilonGreedyBanditHelper();

  EpsilonGreedyBanditHelper(const EpsilonGreedyBanditHelper&) = delete;
  EpsilonGreedyBanditHelper& operator=(const EpsilonGreedyBanditHelper&) =
      delete;

  EpsilonGreedyBanditHelper(EpsilonGreedyBanditHelper&&) noexcept = delete;
  EpsilonGreedyBanditHelper& operator=(EpsilonGreedyBanditHelper&&) noexcept =
      delete;

  ~EpsilonGreedyBanditHelper();

  void Mock();

  static SegmentList Expectation();

 private:
  const EpsilonGreedyBanditProcessor processor_;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_BEHAVIORAL_MULTI_ARMED_BANDITS_EPSILON_GREEDY_BANDIT_UNITTEST_HELPER_H_
