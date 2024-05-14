/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/targeting_unittest_helper.h"

#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"

namespace brave_ads::test {

TargetingHelper::TargetingHelper() = default;

TargetingHelper::~TargetingHelper() = default;

void TargetingHelper::Mock() {
  MockIntent();
  MockLatentInterest();
  MockInterest();
}

// static
UserModelInfo TargetingHelper::Expectation() {
  return UserModelInfo{TargetingHelper::IntentExpectation(),
                       TargetingHelper::LatentInterestExpectation(),
                       TargetingHelper::InterestExpectation()};
}

void TargetingHelper::MockIntent() {
  purchase_intent_.Mock();
}

// static
IntentUserModelInfo TargetingHelper::IntentExpectation() {
  return IntentUserModelInfo{PurchaseIntentHelper::Expectation()};
}

void TargetingHelper::MockLatentInterest() {
  // Intentionally do nothing.
}

// static
LatentInterestUserModelInfo TargetingHelper::LatentInterestExpectation() {
  return LatentInterestUserModelInfo{};
}

void TargetingHelper::MockInterest() {
  text_classification_.Mock();
}

// static
InterestUserModelInfo TargetingHelper::InterestExpectation() {
  return InterestUserModelInfo{TextClassificationHelper::Expectation()};
}

}  // namespace brave_ads::test
