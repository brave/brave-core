/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/country_code/country_code.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/l10n/common/prefs.h"

namespace brave_ads {

namespace {

bool DoesSupportCountryCode() {
  return UserHasJoinedBraveRewards();
}

}  // namespace

CountryCode::CountryCode()
    : cached_country_code_(brave_l10n::GetDefaultISOCountryCodeString()) {
  AddAdsClientNotifierObserver(this);
}

CountryCode::~CountryCode() {
  RemoveAdsClientNotifierObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void CountryCode::OnNotifyDidInitializeAds() {
  CacheCountryCode();
}

void CountryCode::OnNotifyPrefDidChange(const std::string& path) {
  if (path == brave_l10n::prefs::kCountryCode) {
    CacheCountryCode();
  } else if (DoesMatchUserHasJoinedBraveRewardsPrefPath(path)) {
    MaybeSetCountryCode();
  }
}

void CountryCode::OnDidUpdateSubdivision(const std::string& subdivision) {
  std::optional<std::string> subdivision_country_code =
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
  cached_country_code_ =
      GetLocalStateStringPref(brave_l10n::prefs::kCountryCode);
}

void CountryCode::MaybeSetCountryCode() {
  if (DoesSupportCountryCode()) {
    SetLocalStateStringPref(brave_l10n::prefs::kCountryCode,
                            cached_country_code_);
  }
}

}  // namespace brave_ads
