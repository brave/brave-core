/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographical/region/region.h"

#include "brave/components/brave_ads/core/internal/client/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/prefs.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  return UserHasJoinedBraveRewards();
}

}  // namespace

Region::Region()
    : cached_region_code_(brave_l10n::GetDefaultISOCountryCodeString()) {
  AdsClientHelper::AddObserver(this);
}

Region::~Region() {
  AdsClientHelper::RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void Region::OnNotifyPrefDidChange(const std::string& path) {
  if (path == brave_l10n::prefs::kGeoRegionCode) {
    UpdateCachedRegionCode();
  } else if (path == brave_rewards::prefs::kEnabled) {
    MaybeSetRegionCodePref();
  }
}

void Region::OnDidUpdateSubdivision(const std::string& subdivision) {
  absl::optional<std::string> subdivision_country_code =
      GetSubdivisionCountryCode(subdivision);
  if (!subdivision_country_code || subdivision_country_code->empty()) {
    return;
  }

  if (cached_region_code_ == subdivision_country_code) {
    return;
  }
  cached_region_code_ = *subdivision_country_code;

  MaybeSetRegionCodePref();
}

void Region::UpdateCachedRegionCode() {
  absl::optional<base::Value> region_code_value =
      AdsClientHelper::GetInstance()->GetLocalStatePref(
          brave_l10n::prefs::kGeoRegionCode);

  CHECK(region_code_value);
  cached_region_code_ = region_code_value->GetString();
  CHECK(!cached_region_code_.empty());
}

void Region::MaybeSetRegionCodePref() {
  if (DoesRequireResource()) {
    AdsClientHelper::GetInstance()->SetLocalStatePref(
        brave_l10n::prefs::kGeoRegionCode, base::Value(cached_region_code_));
  }
}

}  // namespace brave_ads
