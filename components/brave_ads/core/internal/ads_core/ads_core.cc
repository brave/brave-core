/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_core/ads_core.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_interface.h"

namespace brave_ads {

AdsCore::AdsCore(std::unique_ptr<TokenGeneratorInterface> token_generator)
    : token_generator_(std::move(token_generator)) {
  CHECK(token_generator_);
}

AdsCore::~AdsCore() = default;

const TokenGeneratorInterface* AdsCore::GetTokenGenerator() const {
  return &*token_generator_;
}

Account& AdsCore::GetAccount() {
  return account_;
}

AdHandler& AdsCore::GetAdHandler() {
  return ad_handler_;
}

Reactions& AdsCore::GetReactions() {
  return reactions_;
}

void AdsCore::UpdateReportMetricState(
    const std::string& creative_instance_id,
    mojom::NewTabPageAdMetricType mojom_ad_metric_type) {
  if (mojom_ad_metric_type == mojom::NewTabPageAdMetricType::kDisabled) {
    disable_metrics_.insert(creative_instance_id);
  } else {
    disable_metrics_.erase(creative_instance_id);
  }
}

bool AdsCore::ShouldReportMetric(
    const std::string& creative_instance_id) const {
  return !disable_metrics_.contains(creative_instance_id);
}

}  // namespace brave_ads
