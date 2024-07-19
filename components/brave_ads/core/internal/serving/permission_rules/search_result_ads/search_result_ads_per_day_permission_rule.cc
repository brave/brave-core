/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/search_result_ads/search_result_ads_per_day_permission_rule.h"

#include <cstddef>
#include <optional>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_feature.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_flag_util.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_flag_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_scoring_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/ad_units/search_result_ad/search_result_ad_feature.h"

namespace brave_ads {

bool HasSearchResultAdsPerDayPermission() {
  if (!DoesHistoryRespectRollingTimeConstraint(
          AdType::kSearchResultAd, /*time_constraint=*/base::Days(1),
          /*cap=*/kMaximumSearchResultAdsPerDay.Get())) {
    BLOG(2, "You have exceeded the allowed search result ads per day");
    return false;
  }

  return true;
}

}  // namespace brave_ads
