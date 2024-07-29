/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/locale/locale_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting_constants.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting_util.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_ads {

namespace {

bool DoesRequireResourceForNewTabPageAds() {
  // Require resource only if:
  // - The user has opted into new tab page ads and has either joined Brave
  //   Rewards or new tab page ad events should always be triggered.
  return UserHasOptedInToNewTabPageAds() &&
         (UserHasJoinedBraveRewards() ||
          ShouldAlwaysTriggerNewTabPageAdEvents());
}

bool DoesRequireResource() {
  // Require resource only if:
  // - User has opted into Brave News ads.
  // - User has opted into new tab page ads and either joined Brave Rewards or
  //   new tab page ad events should always be triggered.
  // - User has joined Brave Rewards and opted into notification ads.
  return UserHasOptedInToBraveNewsAds() ||
         DoesRequireResourceForNewTabPageAds() ||
         UserHasOptedInToNotificationAds();
}

}  // namespace

SubdivisionTargeting::SubdivisionTargeting() {
  AddAdsClientNotifierObserver(this);
}

SubdivisionTargeting::~SubdivisionTargeting() {
  RemoveAdsClientNotifierObserver(this);
}

bool SubdivisionTargeting::IsDisabled() const {
  return GetLazyUserSelectedSubdivision() == kSubdivisionTargetingDisabled;
}

bool SubdivisionTargeting::ShouldAutoDetect() const {
  return GetLazyUserSelectedSubdivision() == kSubdivisionTargetingAuto;
}

// static
bool SubdivisionTargeting::ShouldAllow() {
  return GetProfileBooleanPref(prefs::kShouldAllowSubdivisionTargeting);
}

const std::string& SubdivisionTargeting::GetSubdivision() const {
  return ShouldAutoDetect() ? GetLazyAutoDetectedSubdivision()
                            : GetLazyUserSelectedSubdivision();
}

///////////////////////////////////////////////////////////////////////////////

void SubdivisionTargeting::MaybeInitialize() {
  const std::string& auto_detected_subdivision =
      GetLazyAutoDetectedSubdivision();
  std::optional<std::string> country_code =
      GetSubdivisionCountryCode(auto_detected_subdivision);

  if (!country_code) {
    country_code = brave_l10n::GetISOCountryCode(GetLocale());
  }

  MaybeAllowForCountry(*country_code);
}

void SubdivisionTargeting::DisableSubdivision() {
  if (!IsDisabled()) {
    SetUserSelectedSubdivision(kSubdivisionTargetingDisabled);
  }
}

void SubdivisionTargeting::AutoDetectSubdivision() {
  if (!ShouldAutoDetect()) {
    SetUserSelectedSubdivision(kSubdivisionTargetingAuto);
  }
}

void SubdivisionTargeting::MaybeAllowForCountry(
    const std::string& country_code) {
  if (!DoesRequireResource()) {
    return SetProfileBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                 false);
  }

  if (!ShouldTargetSubdivisionCountryCode(country_code)) {
    BLOG(1, "Subdivision targeting is unsupported for " << country_code
                                                        << " country code");
    return SetProfileBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                 false);
  }

  if (IsDisabled()) {
    return SetProfileBooleanPref(prefs::kShouldAllowSubdivisionTargeting, true);
  }

  const std::string& subdivision = GetSubdivision();

  std::optional<std::string> subdivision_country_code;
  if (!subdivision.empty()) {
    subdivision_country_code = GetSubdivisionCountryCode(subdivision);
  }

  if (country_code != subdivision_country_code) {
    AutoDetectSubdivision();

    if (!subdivision_country_code ||
        !ShouldTargetSubdivisionCountryCode(*subdivision_country_code)) {
      return SetProfileBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   false);
    }

    return SetProfileBooleanPref(prefs::kShouldAllowSubdivisionTargeting, true);
  }

  if (!ShouldTargetSubdivision(country_code, subdivision)) {
    BLOG(1, subdivision << " subdivision is unsupported for " << country_code
                        << " country code");

    DisableSubdivision();
  }

  SetProfileBooleanPref(prefs::kShouldAllowSubdivisionTargeting, true);
}

bool SubdivisionTargeting::ShouldFetchSubdivision() {
  if (IsDisabled()) {
    BLOG(1, "Subdivision targeting is disabled");
    return false;
  }

  if (!ShouldAutoDetect()) {
    BLOG(1, "Subdivision targeting is set to "
                << GetLazyUserSelectedSubdivision());
    return false;
  }

  return true;
}

void SubdivisionTargeting::SetAutoDetectedSubdivision(
    const std::string& subdivision) {
  CHECK(!subdivision.empty());

  if (auto_detected_subdivision_ != subdivision) {
    BLOG(1, "Automatically detected " << subdivision << " subdivision");

    auto_detected_subdivision_ = subdivision;
    SetProfileStringPref(prefs::kSubdivisionTargetingAutoDetectedSubdivision,
                         subdivision);
  }
}

void SubdivisionTargeting::UpdateAutoDetectedSubdivision() {
  const std::string auto_detected_subdivision =
      GetProfileStringPref(prefs::kSubdivisionTargetingAutoDetectedSubdivision);

  if (auto_detected_subdivision_ != auto_detected_subdivision) {
    auto_detected_subdivision_ = auto_detected_subdivision;
    BLOG(1, "Changed to automatically detected " << auto_detected_subdivision
                                                 << " subdivision");
  }
}

const std::string& SubdivisionTargeting::GetLazyAutoDetectedSubdivision()
    const {
  if (!auto_detected_subdivision_) {
    auto_detected_subdivision_ = GetProfileStringPref(
        prefs::kSubdivisionTargetingAutoDetectedSubdivision);
  }

  return *auto_detected_subdivision_;
}

void SubdivisionTargeting::SetUserSelectedSubdivision(
    const std::string& subdivision) {
  CHECK(!subdivision.empty());

  if (user_selected_subdivision_ != subdivision) {
    user_selected_subdivision_ = subdivision;
    SetProfileStringPref(prefs::kSubdivisionTargetingSubdivision,
                         *user_selected_subdivision_);
  }
}

void SubdivisionTargeting::UpdateUserSelectedSubdivision() {
  const std::string subdivision =
      GetProfileStringPref(prefs::kSubdivisionTargetingSubdivision);

  if (user_selected_subdivision_ != subdivision) {
    user_selected_subdivision_ = subdivision;
    BLOG(1, "Subdivision changed to " << subdivision);
  }
}

const std::string& SubdivisionTargeting::GetLazyUserSelectedSubdivision()
    const {
  if (!user_selected_subdivision_) {
    user_selected_subdivision_ =
        GetProfileStringPref(prefs::kSubdivisionTargetingSubdivision);
  }

  return *user_selected_subdivision_;
}

void SubdivisionTargeting::OnNotifyDidInitializeAds() {
  MaybeInitialize();
}

void SubdivisionTargeting::OnNotifyPrefDidChange(const std::string& path) {
  if (path == prefs::kSubdivisionTargetingAutoDetectedSubdivision) {
    UpdateAutoDetectedSubdivision();
  } else if (path == prefs::kSubdivisionTargetingSubdivision) {
    UpdateUserSelectedSubdivision();
  } else if (DoesMatchUserHasJoinedBraveRewardsPrefPath(path) ||
             DoesMatchUserHasOptedInToBraveNewsAdsPrefPath(path) ||
             DoesMatchUserHasOptedInToNewTabPageAdsPrefPath(path) ||
             DoesMatchUserHasOptedInToNotificationAdsPrefPath(path)) {
    // This condition should include all the preferences that are present in the
    // `DoesRequireResource` function.
    MaybeInitialize();
  }
}

void SubdivisionTargeting::OnDidUpdateSubdivision(
    const std::string& subdivision) {
  std::optional<std::string> country_code =
      GetSubdivisionCountryCode(subdivision);
  if (!country_code) {
    return;
  }

  SetAutoDetectedSubdivision(subdivision);
  MaybeAllowForCountry(*country_code);
}

}  // namespace brave_ads
