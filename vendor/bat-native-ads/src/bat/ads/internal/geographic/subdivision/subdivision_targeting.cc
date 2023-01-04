/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/locale/subdivision_code_util.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time/time_formatting_util.h"
#include "bat/ads/internal/common/url/url_request_string_util.h"
#include "bat/ads/internal/common/url/url_response_string_util.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/internal/geographic/subdivision/get_subdivision_url_request_builder.h"
#include "bat/ads/internal/geographic/subdivision/supported_subdivision_codes.h"
#include "bat/ads/internal/locale/locale_manager.h"
#include "bat/ads/internal/prefs/pref_manager.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "net/http/http_status_code.h"

namespace ads::geographic {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);
constexpr base::TimeDelta kFetchSubdivisionTargetingPing = base::Days(1);
constexpr base::TimeDelta kDebugFetchSubdivisionTargetingPing =
    base::Minutes(5);
constexpr char kAuto[] = "AUTO";
constexpr char kDisabled[] = "DISABLED";

}  // namespace

SubdivisionTargeting::SubdivisionTargeting() {
  LocaleManager::GetInstance()->AddObserver(this);
  PrefManager::GetInstance()->AddObserver(this);
}

SubdivisionTargeting::~SubdivisionTargeting() {
  LocaleManager::GetInstance()->RemoveObserver(this);
  PrefManager::GetInstance()->RemoveObserver(this);
}

// static
bool SubdivisionTargeting::ShouldAllow() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kShouldAllowSubdivisionTargeting);
}

bool SubdivisionTargeting::IsDisabled() const {
  return GetLazySubdivisionCode() == kDisabled;
}

void SubdivisionTargeting::MaybeAllow() {
  MaybeAllowForLocale(brave_l10n::GetDefaultLocaleString());
}

void SubdivisionTargeting::MaybeFetch() {
  MaybeFetchForLocale(brave_l10n::GetDefaultLocaleString());
}

const std::string& SubdivisionTargeting::GetSubdivisionCode() const {
  if (ShouldAutoDetect()) {
    return GetLazyAutoDetectedSubdivisionCode();
  }

  return GetLazySubdivisionCode();
}

///////////////////////////////////////////////////////////////////////////////

void SubdivisionTargeting::OnAutoDetectedSubdivisionTargetingCodePrefChanged() {
  auto_detected_subdivision_code_ =
      AdsClientHelper::GetInstance()->GetStringPref(
          prefs::kAutoDetectedSubdivisionTargetingCode);
}

void SubdivisionTargeting::OnSubdivisionTargetingCodePrefChanged() {
  const std::string subdivision_code =
      AdsClientHelper::GetInstance()->GetStringPref(
          prefs::kSubdivisionTargetingCode);
  if (subdivision_code_ == subdivision_code) {
    return;
  }

  subdivision_code_ = subdivision_code;

  MaybeFetch();
}

const std::string& SubdivisionTargeting::GetLazyAutoDetectedSubdivisionCode()
    const {
  if (!auto_detected_subdivision_code_) {
    auto_detected_subdivision_code_ =
        AdsClientHelper::GetInstance()->GetStringPref(
            prefs::kAutoDetectedSubdivisionTargetingCode);
  }

  return *auto_detected_subdivision_code_;
}

const std::string& SubdivisionTargeting::GetLazySubdivisionCode() const {
  if (!subdivision_code_) {
    subdivision_code_ = AdsClientHelper::GetInstance()->GetStringPref(
        prefs::kSubdivisionTargetingCode);
  }

  return *subdivision_code_;
}

void SubdivisionTargeting::MaybeAllowForLocale(const std::string& locale) {
  const std::string country_code = brave_l10n::GetISOCountryCode(locale);
  if (!::ads::locale::IsSupportedCountryCodeForSubdivisionTargeting(
          country_code)) {
    AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, false);
    return;
  }

  if (IsDisabled()) {
    AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, true);
    return;
  }

  const std::string& subdivision_code = GetSubdivisionCode();

  std::string subdivision_country_code;
  if (!subdivision_code.empty()) {
    subdivision_country_code = ::ads::locale::GetCountryCode(subdivision_code);
  }
  if (country_code != subdivision_country_code) {
    MaybeResetSubdivisionCodeToAutoDetect();
    AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, false);
    return;
  }

  if (!IsSupportedSubdivisionCode(country_code, subdivision_code)) {
    BLOG(1, "Unknown subdivision code " << subdivision_code << " for " << locale
                                        << " locale ");
    MaybeResetSubdivisionCodeToDisabled();
  }

  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kShouldAllowSubdivisionTargeting, true);
}

void SubdivisionTargeting::MaybeResetSubdivisionCodeToAutoDetect() {
  if (ShouldAutoDetect()) {
    return;
  }

  subdivision_code_ = kAuto;
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kSubdivisionTargetingCode, *subdivision_code_);
}

void SubdivisionTargeting::MaybeResetSubdivisionCodeToDisabled() {
  if (IsDisabled()) {
    return;
  }

  subdivision_code_ = kDisabled;
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kSubdivisionTargetingCode, *subdivision_code_);
}

bool SubdivisionTargeting::ShouldAutoDetect() const {
  return GetLazySubdivisionCode() == kAuto;
}

void SubdivisionTargeting::MaybeFetchForLocale(const std::string& locale) {
  if (retry_timer_.IsRunning()) {
    return;
  }

  const std::string country_code = brave_l10n::GetISOCountryCode(locale);
  if (!::ads::locale::IsSupportedCountryCodeForSubdivisionTargeting(
          country_code)) {
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
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request),
      base::BindOnce(&SubdivisionTargeting::OnFetch, base::Unretained(this)));
}

void SubdivisionTargeting::OnFetch(const mojom::UrlResponseInfo& url_response) {
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

  MaybeAllowForLocale(brave_l10n::GetDefaultLocaleString());

  FetchAfterDelay();
}

bool SubdivisionTargeting::ParseJson(const std::string& json) {
  const absl::optional<base::Value> root = base::JSONReader::Read(json);
  if (!root || !root->is_dict()) {
    return false;
  }
  const base::Value::Dict& dict = root->GetDict();

  const std::string* const country = dict.FindString("country");
  if (!country || country->empty()) {
    return false;
  }

  const std::string* const region = dict.FindString("region");
  if (!region || region->empty()) {
    return false;
  }

  const std::string subdivision_code =
      base::StringPrintf("%s-%s", country->c_str(), region->c_str());

  auto_detected_subdivision_code_ = subdivision_code;
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kAutoDetectedSubdivisionTargetingCode, subdivision_code);

  return true;
}

void SubdivisionTargeting::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&SubdivisionTargeting::Fetch, base::Unretained(this)));

  BLOG(1, "Retry fetching subdivision target "
              << FriendlyDateAndTime(retry_at, /*use_sentence_style*/ true));
}

void SubdivisionTargeting::FetchAfterDelay() {
  const base::TimeDelta delay = FlagManager::GetInstance()->ShouldDebug()
                                    ? kDebugFetchSubdivisionTargetingPing
                                    : kFetchSubdivisionTargetingPing;

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, delay,
      base::BindOnce(&SubdivisionTargeting::Fetch, base::Unretained(this)));

  BLOG(1, "Fetch ads subdivision target "
              << FriendlyDateAndTime(fetch_at, /*use_sentence_style*/ true));
}

void SubdivisionTargeting::OnLocaleDidChange(const std::string& locale) {
  MaybeAllowForLocale(locale);
  MaybeFetchForLocale(locale);
}

void SubdivisionTargeting::OnPrefDidChange(const std::string& path) {
  if (path == prefs::kAutoDetectedSubdivisionTargetingCode) {
    OnAutoDetectedSubdivisionTargetingCodePrefChanged();
  } else if (path == prefs::kSubdivisionTargetingCode) {
    OnSubdivisionTargetingCodePrefChanged();
  }
}

}  // namespace ads::geographic
