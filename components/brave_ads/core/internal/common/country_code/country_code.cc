/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/country_code/country_code.h"

#include "brave/components/brave_ads/core/internal/client/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/prefs.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

bool DoesSupportCountryCode() {
  return UserHasJoinedBraveRewards();
}

}  // namespace

CountryCode::CountryCode()
    : cached_country_code_(brave_l10n::GetDefaultISOCountryCodeString()) {
  AdsClientHelper::AddObserver(this);
}

CountryCode::~CountryCode() {
  AdsClientHelper::RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void CountryCode::OnNotifyDidInitializeAds() {
  CacheCountryCode();
}

void CountryCode::OnNotifyPrefDidChange(const std::string& path) {
  if (path == brave_l10n::prefs::kCountryCode) {
    CacheCountryCode();
  } else if (path == brave_rewards::prefs::kEnabled) {
    MaybeSetCountryCode();
  }
}

void CountryCode::OnDidUpdateSubdivision(const std::string& subdivision) {
  absl::optional<std::string> subdivision_country_code =
      GetSubdivisionCountryCode(subdivision);
  if (!subdivision_country_code || subdivision_country_code->empty()) {
    return;
  }

  if (cached_country_code_ == subdivision_country_code) {
    return;
  }
  cached_country_code_ = *subdivision_country_code;

  MaybeSetCountryCode();
}

void CountryCode::CacheCountryCode() {
  absl::optional<base::Value> country_code_value =
      AdsClientHelper::GetInstance()->GetLocalStatePref(
          brave_l10n::prefs::kCountryCode);

  if (!country_code_value) {
    return;
  }

  CHECK(country_code_value->is_string());
  cached_country_code_ = country_code_value->GetString();
}

void CountryCode::MaybeSetCountryCode() {
  if (DoesSupportCountryCode()) {
    AdsClientHelper::GetInstance()->SetLocalStatePref(
        brave_l10n::prefs::kCountryCode, base::Value(cached_country_code_));
  }
}

}  // namespace brave_ads
