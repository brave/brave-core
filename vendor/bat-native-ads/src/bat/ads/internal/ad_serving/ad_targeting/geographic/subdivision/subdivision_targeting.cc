/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"

#include <cstdint>
#include <functional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/ads.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/get_subdivision_url_request_builder.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/locale/supported_subdivision_codes.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/server/ads_server_util.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "bat/ads/pref_names.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {
namespace ad_targeting {
namespace geographic {

namespace {

const int64_t kRetryAfterSeconds = 1 * base::Time::kSecondsPerMinute;

const int64_t kFetchSubdivisionTargetingPing = 24 * base::Time::kSecondsPerHour;
const int64_t kDebugFetchSubdivisionTargetingPing =
    5 * base::Time::kSecondsPerMinute;

}  // namespace

SubdivisionTargeting::SubdivisionTargeting() = default;

SubdivisionTargeting::~SubdivisionTargeting() = default;

bool SubdivisionTargeting::ShouldAllowForLocale(
    const std::string& locale) const {
  if (!IsSupportedLocale(locale)) {
    return false;
  }

  const std::string country_code = brave_l10n::GetCountryCode(locale);

  const std::string subdivision_targeting_code =
      GetAdsSubdivisionTargetingCode();

  const SupportedSubdivisionCodesSet subdivision_codes =
      kSupportedSubdivisionCodes.at(country_code);
  if (subdivision_codes.find(subdivision_targeting_code) ==
      subdivision_codes.end()) {
    return false;
  }

  return true;
}

bool SubdivisionTargeting::IsDisabled() const {
  const std::string subdivision_targeting_code =
      AdsClientHelper::Get()->GetStringPref(
          prefs::kAdsSubdivisionTargetingCode);

  if (subdivision_targeting_code != "DISABLED") {
    return false;
  }

  return true;
}

void SubdivisionTargeting::MaybeFetchForLocale(const std::string& locale) {
  if (retry_timer_.IsRunning()) {
    return;
  }

  if (!IsSupportedLocale(locale)) {
    BLOG(1, "Ads subdivision targeting is not supported for " << locale
                                                              << " locale");

    AdsClientHelper::Get()->SetBooleanPref(
        prefs::kShouldAllowAdsSubdivisionTargeting, false);

    return;
  }

  if (IsDisabled()) {
    BLOG(1, "Ads subdivision targeting is disabled");
    return;
  }

  if (!ShouldAutoDetect()) {
    const std::string subdivision_targeting_code =
        AdsClientHelper::Get()->GetStringPref(
            prefs::kAdsSubdivisionTargetingCode);

    BLOG(1, "Ads subdivision targeting is enabled for "
                << subdivision_targeting_code);

    return;
  }

  BLOG(1, "Automatically detecting ads subdivision");

  Fetch();
}

void SubdivisionTargeting::MaybeFetchForCurrentLocale() {
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();

  MaybeFetchForLocale(locale);
}

std::string SubdivisionTargeting::GetAdsSubdivisionTargetingCode() const {
  if (ShouldAutoDetect()) {
    return AdsClientHelper::Get()->GetStringPref(
        prefs::kAutoDetectedAdsSubdivisionTargetingCode);
  }

  return AdsClientHelper::Get()->GetStringPref(
      prefs::kAdsSubdivisionTargetingCode);
}

///////////////////////////////////////////////////////////////////////////////

bool SubdivisionTargeting::IsSupportedLocale(const std::string& locale) const {
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  const auto iter = kSupportedSubdivisionCodes.find(country_code);
  if (iter == kSupportedSubdivisionCodes.end()) {
    return false;
  }

  return true;
}

void SubdivisionTargeting::MaybeAllowForLocale(const std::string& locale) {
  const bool should_allow = ShouldAllowForLocale(locale);

  AdsClientHelper::Get()->SetBooleanPref(
      prefs::kShouldAllowAdsSubdivisionTargeting, should_allow);
}

bool SubdivisionTargeting::ShouldAutoDetect() const {
  const std::string subdivision_targeting_code =
      AdsClientHelper::Get()->GetStringPref(
          prefs::kAdsSubdivisionTargetingCode);

  if (subdivision_targeting_code != "AUTO") {
    return false;
  }

  return true;
}

void SubdivisionTargeting::Fetch() {
  BLOG(1, "Fetch subdivision target");
  BLOG(2, "GET /v1/getstate");

  GetSubdivisionUrlRequestBuilder url_request_builder;
  UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(5, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  const auto callback =
      std::bind(&SubdivisionTargeting::OnFetch, this, std::placeholders::_1);
  AdsClientHelper::Get()->UrlRequest(std::move(url_request), callback);
}

void SubdivisionTargeting::OnFetch(const UrlResponse& url_response) {
  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  bool should_retry = false;

  if (url_response.status_code / 100 == 2) {
    BLOG(1, "Successfully fetched subdivision target");

    if (!ParseJson(url_response.body)) {
      BLOG(1, "Failed to parse subdivision target");
      should_retry = true;
    }
  } else {
    BLOG(1, "Failed to fetch subdivision target");
    should_retry = true;
  }

  if (should_retry) {
    Retry();
    return;
  }

  retry_timer_.Stop();

  const std::string subdivision_targeting_code =
      GetAdsSubdivisionTargetingCode();
  BLOG(1, "Automatically detected subdivision targeting code as "
              << subdivision_targeting_code);

  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  MaybeAllowForLocale(locale);

  FetchAfterDelay();
}

bool SubdivisionTargeting::ParseJson(const std::string& json) {
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  const std::string* country = dictionary->FindStringKey("country");
  if (!country || country->empty()) {
    return false;
  }

  const std::string* region = dictionary->FindStringKey("region");
  if (!region || region->empty()) {
    return false;
  }

  const std::string subdivision_code =
      base::StringPrintf("%s-%s", country->c_str(), region->c_str());

  AdsClientHelper::Get()->SetStringPref(
      prefs::kAutoDetectedAdsSubdivisionTargetingCode, subdivision_code);

  return true;
}

void SubdivisionTargeting::Retry() {
  const base::Time time = retry_timer_.StartWithPrivacy(
      base::TimeDelta::FromSeconds(kRetryAfterSeconds),
      base::BindOnce(&SubdivisionTargeting::Fetch, base::Unretained(this)));

  BLOG(1, "Retry fetching subdivision target " << FriendlyDateAndTime(time));
}

void SubdivisionTargeting::FetchAfterDelay() {
  const uint64_t ping = g_is_debug ? kDebugFetchSubdivisionTargetingPing
                                   : kFetchSubdivisionTargetingPing;

  const base::TimeDelta delay = base::TimeDelta::FromSeconds(ping);

  const base::Time time = timer_.StartWithPrivacy(
      delay,
      base::BindOnce(&SubdivisionTargeting::Fetch, base::Unretained(this)));

  BLOG(1, "Fetch ads subdivision target " << FriendlyDateAndTime(time));
}

}  // namespace geographic
}  // namespace ad_targeting
}  // namespace ads
