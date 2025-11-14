/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/policy/handlers/brave_fingerprinting_v2_policy_handler.h"

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "brave/components/constants/pref_names.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"
#include "components/prefs/pref_value_map.h"

namespace policy {

namespace {

// Converts policy values to their corresponding underlying content setting
// values. This abstraction layer allows changing the underlying values while
// keeping the policy values stable.
constexpr auto kPolicyValueToContentSettingMap =
    base::MakeFixedFlatMap<BraveFingerprintingV2Setting, ContentSetting>({
        {BraveFingerprintingV2Setting::kDisableFingerprintingProtection,
         CONTENT_SETTING_ALLOW},
        {BraveFingerprintingV2Setting::
             kEnableFingerprintingProtectionStandardMode,
         CONTENT_SETTING_ASK},
    });

}  // namespace

BraveFingerprintingV2PolicyHandler::BraveFingerprintingV2PolicyHandler()
    : IntRangePolicyHandlerBase(
          key::kDefaultBraveFingerprintingV2Setting,
          static_cast<int>(
              BraveFingerprintingV2Setting::kDisableFingerprintingProtection),
          static_cast<int>(BraveFingerprintingV2Setting::
                               kEnableFingerprintingProtectionStandardMode),
          /*clamp=*/false) {}

BraveFingerprintingV2PolicyHandler::~BraveFingerprintingV2PolicyHandler() =
    default;

void BraveFingerprintingV2PolicyHandler::ApplyPolicySettings(
    const PolicyMap& policies,
    PrefValueMap* prefs) {
  const base::Value* value =
      policies.GetValue(policy_name(), base::Value::Type::INTEGER);

  int value_in_range;
  if (!value || !EnsureInRange(value, &value_in_range, nullptr)) {
    return;
  }

  const auto* content_setting = base::FindOrNull(
      kPolicyValueToContentSettingMap,
      static_cast<BraveFingerprintingV2Setting>(value_in_range));
  if (!content_setting) {
    return;
  }

  CHECK(prefs);
  prefs->SetInteger(kManagedDefaultBraveFingerprintingV2, *content_setting);
}

}  // namespace policy
