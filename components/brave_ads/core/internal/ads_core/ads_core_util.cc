/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_core/ads_core_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_interface.h"
#include "brave/components/brave_ads/core/internal/ads_core/ads_core.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

namespace {

AdsCore& GetAdsCore() {
  CHECK(GlobalState::HasInstance());

  return GlobalState::GetInstance()->GetAdsCore();
}

}  // namespace

const TokenGeneratorInterface* GetTokenGenerator() {
  return GetAdsCore().GetTokenGenerator();
}

Account& GetAccount() {
  return GetAdsCore().GetAccount();
}

AdHandler& GetAdHandler() {
  return GetAdsCore().GetAdHandler();
}

Reactions& GetReactions() {
  return GetAdsCore().GetReactions();
}

void UpdateP3aMetricsFallbackState(const std::string& creative_instance_id,
                                   bool should_metrics_fallback_to_p3a) {
  GetAdsCore().UpdateP3aMetricsFallbackState(creative_instance_id,
                                             should_metrics_fallback_to_p3a);
}

bool ShouldFallbackToP3aMetrics(const std::string& creative_instance_id) {
  return GetAdsCore().ShouldFallbackToP3aMetrics(creative_instance_id);
}

}  // namespace brave_ads
