/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/locale/locale_util.h"
#include "brave/components/brave_ads/core/internal/common/locale/subdivision_code_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_flag_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/get_subdivision_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/get_subdivision_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting_json_reader_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/supported_subdivision_codes.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

namespace {

constexpr char kAuto[] = "AUTO";
constexpr char kDisabled[] = "DISABLED";

constexpr base::TimeDelta kFetchAfter = base::Days(1);
constexpr base::TimeDelta kDebugFetchAfter = base::Minutes(5);
constexpr base::TimeDelta kRetryAfter = base::Minutes(1);

bool ShouldAllowAndFetch() {
  return UserHasOptedIn();
}

}  // namespace

SubdivisionTargeting::SubdivisionTargeting() {
  AdsClientHelper::AddObserver(this);
}

SubdivisionTargeting::~SubdivisionTargeting() {
  AdsClientHelper::RemoveObserver(this);
}

bool SubdivisionTargeting::IsDisabled() const {
  return GetLazySubdivisionCode() == kDisabled;
}

bool SubdivisionTargeting::ShouldAutoDetect() const {
  return GetLazySubdivisionCode() == kAuto;
}

// static
bool SubdivisionTargeting::ShouldAllow() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kShouldAllowSubdivisionTargeting);
}
const std::string& SubdivisionTargeting::GetSubdivisionCode() const {
  return ShouldAutoDetect() ? GetLazyAutoDetectedSubdivisionCode()
                            : GetLazySubdivisionCode();
}

///////////////////////////////////////////////////////////////////////////////

void SubdivisionTargeting::MaybeAllow() {
  if (ShouldAllowAndFetch()) {
    MaybeAllowForLocale(GetLocale());
  }
}

void SubdivisionTargeting::MaybeFetch() {
  if (ShouldAllowAndFetch()) {
    MaybeFetchForLocale(GetLocale());
  }
}

void SubdivisionTargeting::MaybeAllowAndFetch() {
  if (!did_fetch_) {
    MaybeAllow();
    MaybeFetch();
  }
}

void SubdivisionTargeting::SetAutoDetectedSubdivisionCode(
    const std::string& subdivision_code) {
  CHECK(!subdivision_code.empty());

  auto_detected_subdivision_code_ = subdivision_code;
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kAutoDetectedSubdivisionTargetingCode, subdivision_code);
}

void SubdivisionTargeting::UpdateAutoDetectedSubdivisionCode() {
  auto_detected_subdivision_code_ =
      AdsClientHelper::GetInstance()->GetStringPref(
          prefs::kAutoDetectedSubdivisionTargetingCode);
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

void SubdivisionTargeting::SetSubdivisionCode(
    const std::string& subdivision_code) {
  CHECK(!subdivision_code.empty());

  subdivision_code_ = subdivision_code;
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kSubdivisionTargetingCode, *subdivision_code_);
}

void SubdivisionTargeting::UpdateSubdivisionCode() {
  const std::string subdivision_code =
      AdsClientHelper::GetInstance()->GetStringPref(
          prefs::kSubdivisionTargetingCode);

  if (subdivision_code_ != subdivision_code) {
    subdivision_code_ = subdivision_code;
    MaybeFetch();
  }
}

const std::string& SubdivisionTargeting::GetLazySubdivisionCode() const {
  if (!subdivision_code_) {
    subdivision_code_ = AdsClientHelper::GetInstance()->GetStringPref(
        prefs::kSubdivisionTargetingCode);
  }

  return *subdivision_code_;
}

void SubdivisionTargeting::MaybeDisable() {
  if (!IsDisabled()) {
    SetSubdivisionCode(kDisabled);
  }
}

void SubdivisionTargeting::MaybeAutoDetectSubdivisionCode() {
  if (!ShouldAutoDetect()) {
    SetSubdivisionCode(kAuto);
  }
}

void SubdivisionTargeting::MaybeAllowForLocale(const std::string& locale) {
  const std::string country_code = brave_l10n::GetISOCountryCode(locale);

  if (!DoesSupportSubdivisionTargeting(country_code)) {
    return AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, false);
  }

  if (IsDisabled()) {
    return AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, true);
  }

  const std::string& subdivision_code = GetSubdivisionCode();

  std::string subdivision_country_code;
  if (!subdivision_code.empty()) {
    subdivision_country_code = GetCountryCode(subdivision_code);
  }

  if (country_code != subdivision_country_code) {
    MaybeAutoDetectSubdivisionCode();

    return AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, false);
  }

  if (!IsSupportedSubdivisionCode(country_code, subdivision_code)) {
    BLOG(1, subdivision_code << " subdivision code is unsupported for "
                             << country_code << " country code");

    MaybeDisable();
  }

  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kShouldAllowSubdivisionTargeting, true);
}

void SubdivisionTargeting::MaybeFetchForLocale(const std::string& locale) {
  const std::string country_code = brave_l10n::GetISOCountryCode(locale);
  if (!DoesSupportSubdivisionTargeting(country_code)) {
    BLOG(1, "Subdivision targeting is unsupported for " << country_code
                                                        << " country code");

    return AdsClientHelper::GetInstance()->SetBooleanPref(
        prefs::kShouldAllowSubdivisionTargeting, false);
  }

  if (IsDisabled()) {
    return BLOG(1, "Subdivision targeting is disabled");
  }

  if (!ShouldAutoDetect()) {
    return BLOG(
        1, "Subdivision targeting is enabled for " << GetLazySubdivisionCode());
  }

  BLOG(1, "Automatically detected " << GetLazyAutoDetectedSubdivisionCode()
                                    << " subdivision targeting code");

  Fetch();
}

void SubdivisionTargeting::FetchAfterDelay() {
  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, ShouldDebug() ? kDebugFetchAfter : kFetchAfter,
      base::BindOnce(&SubdivisionTargeting::Fetch, weak_factory_.GetWeakPtr()));

  BLOG(1, "Fetch subdivision target " << FriendlyDateAndTime(fetch_at));
}

void SubdivisionTargeting::Fetch() {
  if (is_fetching_ || retry_timer_.IsRunning()) {
    return;
  }

  BLOG(1, "FetchSubdivisionTargeting " << BuildSubdivisionTargetingUrlPath());

  is_fetching_ = true;

  GetSubdivisionUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request),
      base::BindOnce(&SubdivisionTargeting::FetchCallback,
                     weak_factory_.GetWeakPtr()));
}

void SubdivisionTargeting::FetchCallback(
    const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnFetchSubdivisionTargeting");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  is_fetching_ = false;

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to fetch subdivision targeting code");
    return Retry();
  }

  BLOG(1, "Successfully fetched subdivision targeting code");

  const absl::optional<std::string> subdivision_code =
      json::reader::ParseSubdivisionCode(url_response.body);
  if (!subdivision_code) {
    BLOG(1, "Failed to parse subdivision targeting code");
    return Retry();
  }

  StopRetrying();
  SetAutoDetectedSubdivisionCode(*subdivision_code);

  MaybeAllowForLocale(GetLocale());

  did_fetch_ = true;

  FetchAfterDelay();
}

void SubdivisionTargeting::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&SubdivisionTargeting::RetryCallback,
                     weak_factory_.GetWeakPtr()));

  BLOG(1,
       "Retry fetching subdivision target " << FriendlyDateAndTime(retry_at));
}

void SubdivisionTargeting::RetryCallback() {
  BLOG(1, "Retry fetching subdivision target");

  Fetch();
}

void SubdivisionTargeting::StopRetrying() {
  retry_timer_.Stop();
}

void SubdivisionTargeting::OnNotifyDidInitializeAds() {
  MaybeAllow();
  MaybeFetch();
}

void SubdivisionTargeting::OnNotifyLocaleDidChange(const std::string& locale) {
  MaybeAllowForLocale(locale);
  MaybeFetchForLocale(locale);
}

void SubdivisionTargeting::OnNotifyPrefDidChange(const std::string& path) {
  if (path == prefs::kEnabled || path == brave_news::prefs::kBraveNewsOptedIn ||
      path == brave_news::prefs::kNewTabPageShowToday) {
    MaybeAllowAndFetch();
  } else if (path == prefs::kAutoDetectedSubdivisionTargetingCode) {
    UpdateAutoDetectedSubdivisionCode();
  } else if (path == prefs::kSubdivisionTargetingCode) {
    UpdateSubdivisionCode();
  }
}

}  // namespace brave_ads
