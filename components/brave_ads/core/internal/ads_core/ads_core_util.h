/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CORE_ADS_CORE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CORE_ADS_CORE_UTIL_H_

#include <string>

#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_handler.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"

namespace brave_ads {

class TokenGeneratorInterface;

// Provides methods for generating tokens for refilling and redeeming
// confirmation tokens.
const TokenGeneratorInterface* GetTokenGenerator();

// Provides methods for managing user rewards, wallets, account statements, and
// deposits.
Account& GetAccount();

// Provides methods for serving and triggering events for various types of ads,
// including inline content ads, new tab page ads, notification ads, promoted
// content ads, and search result ads.
AdHandler& GetAdHandler();

// Provides methods for engaging with ads, such as liking, disliking, marking as
// inappropriate, and saving ads.
Reactions& GetReactions();

// Updates the reporting state for a creative instance ID, enabling or
// disabling metrics reporting.
void UpdateReportMetricState(
    const std::string& creative_instance_id,
    mojom::NewTabPageAdMetricType mojom_ad_metric_type);

// Returns true if metrics should be reported for the given creative instance
// ID, otherwise false.
bool ShouldReportMetric(const std::string& creative_instance_id);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CORE_ADS_CORE_UTIL_H_
