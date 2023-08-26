/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"

#include "brave/components/brave_ads/core/internal/client/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/locale/locale_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting_constants.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_ads {

namespace {

bool DoesRequireResource() {
  return UserHasOptedInToBraveNewsAds() || UserHasOptedInToNotificationAds();
}

}  // namespace

SubdivisionTargeting::SubdivisionTargeting() {
  AdsClientHelper::AddObserver(this);
}

SubdivisionTargeting::~SubdivisionTargeting() {
  AdsClientHelper::RemoveObserver(this);
}

bool SubdivisionTargeting::IsDisabled() const {
  return GetLazyUserSelectedSubdivision() == kSubdivisionTargetingDisabled;
}

bool SubdivisionTargeting::ShouldAutoDetect() const {
  return GetLazyUserSelectedSubdivision() == kSubdivisionTargetingAuto;
}

// static
bool SubdivisionTargeting::ShouldAllow() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kShouldAllowSubdivisionTargeting);
}

const std::string& SubdivisionTargeting::GetSubdivision() const {
  return ShouldAutoDetect() ? GetLazyAutoDetectedSubdivision()
                            : GetLazyUserSelectedSubdivision();
}

///////////////////////////////////////////////////////////////////////////////

void SubdivisionTargeting::Initialize() {
  const std::string& auto_detected_subdivision =
      GetLazyAutoDetectedSubdivision();
  absl::optional<std::string> country_code =
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
    return AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, false);
  }

  if (!ShouldTargetSubdivisionCountryCode(country_code)) {
    BLOG(1, "Subdivision targeting is unsupported for " << country_code
                                                        << " country code");

    return AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, false);
  }

  if (IsDisabled()) {
    return AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, true);
  }

  const std::string& subdivision = GetSubdivision();

  absl::optional<std::string> subdivision_country_code;
  if (!subdivision.empty()) {
    subdivision_country_code = GetSubdivisionCountryCode(subdivision);
  }

  if (country_code != subdivision_country_code) {
    AutoDetectSubdivision();

    if (!subdivision_country_code ||
        !ShouldTargetSubdivisionCountryCode(*subdivision_country_code)) {
      return AdsClientHelper::GetInstance()->SetBooleanPref(
          prefs::kShouldAllowSubdivisionTargeting, false);
    }

    return AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, true);
  }

  if (!ShouldTargetSubdivision(country_code, subdivision)) {
    BLOG(1, subdivision << " subdivision is unsupported for " << country_code
                        << " country code");

    DisableSubdivision();
  }

  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kShouldAllowSubdivisionTargeting, true);
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
    AdsClientHelper::GetInstance()->SetStringPref(
        prefs::kSubdivisionTargetingAutoDetectedSubdivision, subdivision);
  }
}

void SubdivisionTargeting::UpdateAutoDetectedSubdivision() {
  const std::string auto_detected_subdivision =
      AdsClientHelper::GetInstance()->GetStringPref(
          prefs::kSubdivisionTargetingAutoDetectedSubdivision);

  if (auto_detected_subdivision_ != auto_detected_subdivision) {
    auto_detected_subdivision_ = auto_detected_subdivision;
    BLOG(1, "Changed to automatically detected " << auto_detected_subdivision
                                                 << " subdivision");
  }
}

const std::string& SubdivisionTargeting::GetLazyAutoDetectedSubdivision()
    const {
  if (!auto_detected_subdivision_) {
    auto_detected_subdivision_ = AdsClientHelper::GetInstance()->GetStringPref(
        prefs::kSubdivisionTargetingAutoDetectedSubdivision);
  }

  return *auto_detected_subdivision_;
}

void SubdivisionTargeting::SetUserSelectedSubdivision(
    const std::string& subdivision) {
  CHECK(!subdivision.empty());

  if (user_selected_subdivision_ != subdivision) {
    user_selected_subdivision_ = subdivision;
    AdsClientHelper::GetInstance()->SetStringPref(
        prefs::kSubdivisionTargetingSubdivision, *user_selected_subdivision_);
  }
}

void SubdivisionTargeting::UpdateUserSelectedSubdivision() {
  const std::string subdivision = AdsClientHelper::GetInstance()->GetStringPref(
      prefs::kSubdivisionTargetingSubdivision);

  if (user_selected_subdivision_ != subdivision) {
    user_selected_subdivision_ = subdivision;
    BLOG(1, "Subdivision changed to " << subdivision);
  }
}

const std::string& SubdivisionTargeting::GetLazyUserSelectedSubdivision()
    const {
  if (!user_selected_subdivision_) {
    user_selected_subdivision_ = AdsClientHelper::GetInstance()->GetStringPref(
        prefs::kSubdivisionTargetingSubdivision);
  }

  return *user_selected_subdivision_;
}

void SubdivisionTargeting::OnNotifyDidInitializeAds() {
  Initialize();
}

void SubdivisionTargeting::OnNotifyPrefDidChange(const std::string& path) {
  if (path == prefs::kSubdivisionTargetingAutoDetectedSubdivision) {
    UpdateAutoDetectedSubdivision();
  } else if (path == prefs::kSubdivisionTargetingSubdivision) {
    UpdateUserSelectedSubdivision();
  }
}

void SubdivisionTargeting::OnDidUpdateSubdivision(
    const std::string& subdivision) {
  absl::optional<std::string> country_code =
      GetSubdivisionCountryCode(subdivision);
  if (!country_code) {
    return;
  }

  SetAutoDetectedSubdivision(subdivision);
  MaybeAllowForCountry(*country_code);
}

}  // namespace brave_ads
