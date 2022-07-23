/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"

#include <cstdint>
#include <functional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/net/http/http_status_code.h"
#include "bat/ads/internal/base/time/time_formatting_util.h"
#include "bat/ads/internal/base/url/url_request_string_util.h"
#include "bat/ads/internal/base/url/url_response_string_util.h"
#include "bat/ads/internal/geographic/subdivision/get_subdivision_url_request_builder.h"
#include "bat/ads/internal/geographic/subdivision/supported_subdivision_codes.h"
#include "bat/ads/internal/locale/locale_manager.h"
#include "bat/ads/internal/prefs/pref_manager.h"
#include "bat/ads/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {
namespace geographic {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);
constexpr base::TimeDelta kFetchSubdivisionTargetingPing = base::Days(1);
constexpr base::TimeDelta kDebugFetchSubdivisionTargetingPing =
    base::Minutes(5);

}  // namespace

SubdivisionTargeting::SubdivisionTargeting() {
  LocaleManager::GetInstance()->AddObserver(this);
  PrefManager::GetInstance()->AddObserver(this);
}

SubdivisionTargeting::~SubdivisionTargeting() {
  LocaleManager::GetInstance()->RemoveObserver(this);
  PrefManager::GetInstance()->RemoveObserver(this);
}

bool SubdivisionTargeting::ShouldAllowForLocale(
    const std::string& locale) const {
  if (!IsSupportedLocale(locale)) {
    return false;
  }

  const std::string country_code = brave_l10n::GetCountryCode(locale);
  const SupportedSubdivisionCodesSet subdivision_codes =
      kSupportedSubdivisionCodes.at(country_code);

  const std::string subdivision_code = GetSubdivisionCode();
  if (subdivision_codes.find(subdivision_code) == subdivision_codes.end()) {
    return false;
  }

  return true;
}

bool SubdivisionTargeting::IsDisabled() const {
  if (GetLazySubdivisionCode() != "DISABLED") {
    return false;
  }

  return true;
}

void SubdivisionTargeting::MaybeFetch() {
  const std::string locale = LocaleManager::GetInstance()->GetLocale();
  MaybeFetchForLocale(locale);
}

std::string SubdivisionTargeting::GetSubdivisionCode() const {
  if (ShouldAutoDetect()) {
    return GetLazyAutoDetectedSubdivisionCode();
  }

  return GetLazySubdivisionCode();
}

///////////////////////////////////////////////////////////////////////////////

void SubdivisionTargeting::OnAutoDetectedSubdivisionTargetingCodePrefChanged() {
  auto_detected_subdivision_code_optional_ =
      AdsClientHelper::GetInstance()->GetStringPref(
          prefs::kAutoDetectedSubdivisionTargetingCode);
}

void SubdivisionTargeting::OnSubdivisionTargetingCodePrefChanged() {
  subdivision_code_optional_ = AdsClientHelper::GetInstance()->GetStringPref(
      prefs::kSubdivisionTargetingCode);

  MaybeFetch();
}

std::string SubdivisionTargeting::GetLazyAutoDetectedSubdivisionCode() const {
  if (!auto_detected_subdivision_code_optional_) {
    auto_detected_subdivision_code_optional_ =
        AdsClientHelper::GetInstance()->GetStringPref(
            prefs::kAutoDetectedSubdivisionTargetingCode);
  }

  return auto_detected_subdivision_code_optional_.value();
}

std::string SubdivisionTargeting::GetLazySubdivisionCode() const {
  if (!subdivision_code_optional_) {
    subdivision_code_optional_ = AdsClientHelper::GetInstance()->GetStringPref(
        prefs::kSubdivisionTargetingCode);
  }

  return subdivision_code_optional_.value();
}

bool SubdivisionTargeting::IsSupportedLocale(const std::string& locale) const {
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  const auto iter = kSupportedSubdivisionCodes.find(country_code);
  if (iter == kSupportedSubdivisionCodes.end()) {
    return false;
  }

  return true;
}

bool SubdivisionTargeting::ShouldAutoDetect() const {
  if (GetLazySubdivisionCode() != "AUTO") {
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

    AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, false);

    return;
  }

  if (IsDisabled()) {
    BLOG(1, "Ads subdivision targeting is disabled");
    return;
  }

  if (!ShouldAutoDetect()) {
    BLOG(1, "Ads subdivision targeting is enabled for "
                << GetLazySubdivisionCode());

    return;
  }

  BLOG(1, "Automatically detecting ads subdivision");

  Fetch();
}

void SubdivisionTargeting::Fetch() {
  BLOG(1, "FetchSubdivisionTargeting");
  BLOG(2, "GET /v1/getstate");

  GetSubdivisionUrlRequestBuilder url_request_builder;
  mojom::UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  const auto callback =
      std::bind(&SubdivisionTargeting::OnFetch, this, std::placeholders::_1);
  AdsClientHelper::GetInstance()->UrlRequest(std::move(url_request), callback);
}

void SubdivisionTargeting::OnFetch(const mojom::UrlResponse& url_response) {
  BLOG(1, "OnFetchSubdivisionTargeting");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to fetch subdivision target");
    Retry();
    return;
  }

  BLOG(1, "Successfully fetched subdivision target");

  if (!ParseJson(url_response.body)) {
    BLOG(1, "Failed to parse subdivision target");
    Retry();
    return;
  }

  retry_timer_.Stop();

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

  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kAutoDetectedSubdivisionTargetingCode, subdivision_code);

  return true;
}

void SubdivisionTargeting::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&SubdivisionTargeting::Fetch, base::Unretained(this)));

  BLOG(1,
       "Retry fetching subdivision target " << FriendlyDateAndTime(retry_at));
}

void SubdivisionTargeting::FetchAfterDelay() {
  const base::TimeDelta delay = g_is_debug ? kDebugFetchSubdivisionTargetingPing
                                           : kFetchSubdivisionTargetingPing;

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, delay,
      base::BindOnce(&SubdivisionTargeting::Fetch, base::Unretained(this)));

  BLOG(1, "Fetch ads subdivision target " << FriendlyDateAndTime(fetch_at));
}

void SubdivisionTargeting::OnLocaleDidChange(const std::string& locale) {
  MaybeFetchForLocale(locale);
}

void SubdivisionTargeting::OnPrefChanged(const std::string& path) {
  if (path == prefs::kAutoDetectedSubdivisionTargetingCode) {
    OnAutoDetectedSubdivisionTargetingCodePrefChanged();
  } else if (path == prefs::kSubdivisionTargetingCode) {
    OnSubdivisionTargetingCodePrefChanged();
  }
}

}  // namespace geographic
}  // namespace ads
