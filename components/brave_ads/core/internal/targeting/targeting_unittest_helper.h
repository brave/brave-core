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

namespace test {

class TargetingHelper final {
 public:
  TargetingHelper();

  TargetingHelper(const TargetingHelper&) = delete;
  TargetingHelper& operator=(const TargetingHelper&) = delete;

  TargetingHelper(TargetingHelper&&) noexcept = delete;
  TargetingHelper& operator=(TargetingHelper&&) noexcept = delete;

  ~TargetingHelper();

  void Mock();
  static UserModelInfo Expectation();

  void MockIntent();
  static IntentUserModelInfo IntentExpectation();

  void MockLatentInterest();
  static LatentInterestUserModelInfo LatentInterestExpectation();

  void MockInterest();
  static InterestUserModelInfo InterestExpectation();

 private:
  EpsilonGreedyBanditHelper epsilon_greedy_bandit_;
  PurchaseIntentHelper purchase_intent_;
  TextClassificationHelper text_classification_;
  TextEmbeddingHelper text_embedding_;
};

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_TARGETING_UNITTEST_HELPER_H_
