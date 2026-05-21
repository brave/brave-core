/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/test/targeting_test_helper.h"

#include "base/check.h"
#include "base/test/run_until.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/intent/intent_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/interest/interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/latent_interest/latent_interest_user_model_info.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/user_model/user_model_info.h"

namespace brave_ads::test {

TargetingHelper::TargetingHelper() = default;

TargetingHelper::~TargetingHelper() = default;

void TargetingHelper::Simulate() {
  SimulateIntent();
  SimulateLatentInterest();
  SimulateInterest();
}

// static
UserModelInfo TargetingHelper::Expectation() {
  return UserModelInfo{TargetingHelper::IntentExpectation(),
                       TargetingHelper::LatentInterestExpectation(),
                       TargetingHelper::InterestExpectation()};
}

void TargetingHelper::SimulateIntent() {
  purchase_intent_.Simulate();
}

// static
IntentUserModelInfo TargetingHelper::IntentExpectation() {
  return IntentUserModelInfo{PurchaseIntentHelper::Expectation()};
}

void TargetingHelper::SimulateLatentInterest() {
  // Intentionally do nothing.
}

// static
LatentInterestUserModelInfo TargetingHelper::LatentInterestExpectation() {
  return LatentInterestUserModelInfo{};
}

void TargetingHelper::SimulateInterest() {
  text_classification_.Simulate();

  CHECK(base::test::RunUntil([] {
    return ClientStateManager::GetInstance()
               .GetTextClassificationProbabilitiesHistory()
               .size() == 3U;
  }));
}

// static
InterestUserModelInfo TargetingHelper::InterestExpectation() {
  return InterestUserModelInfo{TextClassificationHelper::Expectation()};
}

}  // namespace brave_ads::test
