/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_TARGETING_UNITTEST_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_TARGETING_UNITTEST_HELPER_H_

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_unittest_helper.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_unittest_helper.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/text_classification_unittest_helper.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_unittest_helper.h"

namespace brave_ads {

struct IntentUserModelInfo;
struct InterestUserModelInfo;
struct LatentInterestUserModelInfo;
struct UserModelInfo;

class TargetingHelperForTesting final {
 public:
  TargetingHelperForTesting();

  TargetingHelperForTesting(const TargetingHelperForTesting&) = delete;
  TargetingHelperForTesting& operator=(const TargetingHelperForTesting&) =
      delete;

  TargetingHelperForTesting(TargetingHelperForTesting&&) noexcept = delete;
  TargetingHelperForTesting& operator=(TargetingHelperForTesting&&) noexcept =
      delete;

  ~TargetingHelperForTesting();

  void Mock();
  static UserModelInfo Expectation();

  void MockIntent();
  static IntentUserModelInfo IntentExpectation();

  void MockLatentInterest();
  static LatentInterestUserModelInfo LatentInterestExpectation();

  void MockInterest();
  static InterestUserModelInfo InterestExpectation();

 private:
  EpsilonGreedyBanditHelperForTesting epsilon_greedy_bandit_;
  PurchaseIntentHelperForTesting purchase_intent_;
  TextClassificationHelperForTesting text_classification_;
  TextEmbeddingHelperForTesting text_embedding_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_TARGETING_UNITTEST_HELPER_H_
