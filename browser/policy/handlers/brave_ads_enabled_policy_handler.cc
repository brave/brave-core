/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/handlers/brave_ads_enabled_policy_handler.h"

#include "base/values.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"

namespace policy {

BraveAdsEnabledPolicyHandler::BraveAdsEnabledPolicyHandler()
    : TypeCheckingPolicyHandler(key::kBraveAdsEnabled,
                                base::Value::Type::BOOLEAN) {}

BraveAdsEnabledPolicyHandler::~BraveAdsEnabledPolicyHandler() = default;

void BraveAdsEnabledPolicyHandler::ApplyPolicySettings(
    const PolicyMap& policies,
    PrefValueMap* prefs) {
  const base::Value* value =
      policies.GetValue(policy_name(), base::Value::Type::BOOLEAN);
  if (!value || value->GetBool()) {
    // When the policy is `true` or unset, users control each ad type opt-in
    // independently.
    return;
  }

  CHECK(prefs);
  prefs->SetBoolean(brave_ads::prefs::kOptedInToNotificationAds, false);
  prefs->SetBoolean(brave_ads::prefs::kOptedInToSearchResultAds, false);
  prefs->SetBoolean(ntp_background_images::prefs::
                        kNewTabPageShowSponsoredImagesBackgroundImage,
                    false);
}

}  // namespace policy
