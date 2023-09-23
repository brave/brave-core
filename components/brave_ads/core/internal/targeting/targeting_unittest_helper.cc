/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/targeting_unittest_helper.h"

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"

namespace brave_ads {

TargetingHelperForTesting::TargetingHelperForTesting() = default;

TargetingHelperForTesting::~TargetingHelperForTesting() = default;

void TargetingHelperForTesting::Mock() {
  MockIntent();
  MockLatentInterest();
  MockInterest();
}

// static
UserModelInfo TargetingHelperForTesting::Expectation() {
  return UserModelInfo{TargetingHelperForTesting::IntentExpectation(),
                       TargetingHelperForTesting::LatentInterestExpectation(),
                       TargetingHelperForTesting::InterestExpectation()};
}

void TargetingHelperForTesting::MockIntent() {
  purchase_intent_.Mock();
}

// static
IntentUserModelInfo TargetingHelperForTesting::IntentExpectation() {
  return IntentUserModelInfo{PurchaseIntentHelperForTesting::Expectation()};
}

void TargetingHelperForTesting::MockLatentInterest() {
  epsilon_greedy_bandit_.Mock();
}

// static
LatentInterestUserModelInfo
TargetingHelperForTesting::LatentInterestExpectation() {
  return LatentInterestUserModelInfo{
      EpsilonGreedyBanditHelperForTesting::Expectation()};
}

void TargetingHelperForTesting::MockInterest() {
  text_classification_.Mock();
  text_embedding_.Mock();
}

// static
InterestUserModelInfo TargetingHelperForTesting::InterestExpectation() {
  return InterestUserModelInfo{
      TextClassificationHelperForTesting::Expectation(),
      TextEmbeddingHelperForTesting::Expectation()};
}

}  // namespace brave_ads
