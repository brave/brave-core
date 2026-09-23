/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/handlers/brave_adblock_policy_handler.h"

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"

namespace policy {

namespace {

// Converts policy values to their corresponding underlying content setting
// values. This abstraction layer allows changing the underlying values while
// keeping the policy values stable.
constexpr auto kPolicyValueToContentSettingMap =
    base::MakeFixedFlatMap<BraveAdblockSetting, ContentSetting>({
        {BraveAdblockSetting::kAllowAds, CONTENT_SETTING_ALLOW},
        {BraveAdblockSetting::kBlockAds, CONTENT_SETTING_BLOCK},
    });

}  // namespace

BraveAdblockPolicyHandler::BraveAdblockPolicyHandler()
    : IntRangePolicyHandlerBase(
          key::kDefaultBraveAdblockSetting,
          static_cast<int>(BraveAdblockSetting::kAllowAds),
          static_cast<int>(BraveAdblockSetting::kBlockAds),
          /*clamp=*/false) {}

BraveAdblockPolicyHandler::~BraveAdblockPolicyHandler() = default;

void BraveAdblockPolicyHandler::ApplyPolicySettings(const PolicyMap& policies,
                                                    PrefValueMap* prefs) {
  const base::Value* value =
      policies.GetValue(policy_name(), base::Value::Type::INTEGER);

  int value_in_range;
  if (!value || !EnsureInRange(value, &value_in_range, nullptr)) {
    return;
  }

  const auto* content_setting =
      base::FindOrNull(kPolicyValueToContentSettingMap,
                       static_cast<BraveAdblockSetting>(value_in_range));
  if (!content_setting) {
    return;
  }

  CHECK(prefs);
  prefs->SetInteger(prefs::kManagedDefaultBraveAdblockSetting,
                    *content_setting);
}

}  // namespace policy
