/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/subdivision_targeting.h"

#include <stdint.h>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/subdivision_targeting_codes.h"
#include "bat/ads/internal/time_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ads {

SubdivisionTargeting::SubdivisionTargeting(
    AdsClient* ads_client)
    : ads_client_(ads_client) {
  DCHECK(ads_client_);
  BuildUrl();
}

SubdivisionTargeting::~SubdivisionTargeting() = default;

bool SubdivisionTargeting::ShouldAllowAdsSubdivisionTargeting(
    const std::string& locale) const {
  if (!IsSupportedLocale(locale)) {
    return false;
  }

  const std::string region = brave_l10n::GetRegionCode(locale);

  const std::string subdivision_targeting_code =
      GetAdsSubdivisionTargetingCode();

  const SubdivisionTargetingCodesSet subdivision_targeting_codes =
      kSubdivisionTargetingCodes.at(region);
  if (subdivision_targeting_codes.find(subdivision_targeting_code) ==
      subdivision_targeting_codes.end()) {
    return false;
  }

  return true;
}

bool SubdivisionTargeting::IsDisabled() const {
  const std::string subdivision_targeting_code =
      ads_client_->GetAdsSubdivisionTargetingCode();

  if (subdivision_targeting_code != "DISABLED") {
    return false;
  }

  return true;
}

void SubdivisionTargeting::MaybeFetch(
    const std::string& locale) {
  if (!IsSupportedLocale(locale)) {
    BLOG(1, "Ads subdivision targeting is not supported for " << locale
        << " locale");

    ads_client_->SetAllowAdsSubdivisionTargeting(false);
    return;
  }

  if (IsDisabled()) {
    BLOG(1, "Ads subdivision targeting is disabled");
    return;
  }

  if (!ShouldAutomaticallyDetect()) {
    const std::string subdivision_targeting_code =
        ads_client_->GetAdsSubdivisionTargetingCode();

    BLOG(1, "Ads subdivision targeting is enabled for "
        << subdivision_targeting_code);

    return;
  }

  BLOG(1, "Automatically detecting ads subdivision");

  Fetch();
}

std::string SubdivisionTargeting::GetAdsSubdivisionTargetingCode() const {
  if (ShouldAutomaticallyDetect()) {
    return ads_client_->GetAutomaticallyDetectedAdsSubdivisionTargetingCode();
  }

  return ads_client_->GetAdsSubdivisionTargetingCode();
}

///////////////////////////////////////////////////////////////////////////////

bool SubdivisionTargeting::IsSupportedLocale(
    const std::string& locale) const {
  const std::string region = brave_l10n::GetRegionCode(locale);

  const auto iter = kSubdivisionTargetingCodes.find(region);
  if (iter == kSubdivisionTargetingCodes.end()) {
    return false;
  }

  return true;
}

void SubdivisionTargeting::MaybeAllowAdsSubdivisionTargetingForLocale(
    const std::string& locale) {
  const bool should_allow = ShouldAllowAdsSubdivisionTargeting(locale);
  ads_client_->SetAllowAdsSubdivisionTargeting(should_allow);
}

bool SubdivisionTargeting::ShouldAutomaticallyDetect() const {
  const std::string subdivision_targeting_code =
      ads_client_->GetAdsSubdivisionTargetingCode();

  if (subdivision_targeting_code != "AUTO") {
    return false;
  }

  return true;
}

void SubdivisionTargeting::Fetch() {
  if (retry_timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Fetch ads subdivision");
  BLOG(2, "GET " << GETSTATE_PATH);

  const auto callback =
      std::bind(&SubdivisionTargeting::OnFetch, this, url_, _1, _2, _3);

  BLOG(5, UrlRequestToString(url_, {}, "", "", URLRequestMethod::GET));
  ads_client_->URLRequest(url_, {}, "", "", URLRequestMethod::GET, callback);
}

void SubdivisionTargeting::OnFetch(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  BLOG(6, UrlResponseToString(url, response_status_code, response, headers));

  bool should_retry = false;

  if (response_status_code / 100 == 2) {
    if (!response.empty()) {
      BLOG(1, "Successfully fetched ads subdivision");
    }

    if (!ParseJson(response)) {
      BLOG(1, "Failed to parse ads subdivision");
      should_retry = true;
    }
  } else if (response_status_code == 304) {
    BLOG(1, "Ads subdivision is up to date");
  } else {
    BLOG(1, "Failed to fetch ads subdivision");

    should_retry = true;
  }

  if (should_retry) {
    Retry();
    return;
  }

  retry_timer_.Stop();

  const std::string subdivision_targeting_code =
      GetAdsSubdivisionTargetingCode();

  BLOG(1, "Automatically detected ads subdivision targeting code as "
      << subdivision_targeting_code);

  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  MaybeAllowAdsSubdivisionTargetingForLocale(locale);

  FetchAfterDelay();
}

bool SubdivisionTargeting::ParseJson(
    const std::string& json) {
  base::Optional<base::Value> value = base::JSONReader::Read(json);
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

  std::string subdivision_targeting_code =
      base::StringPrintf("%s-%s", country->c_str(), region->c_str());

  ads_client_->SetAutomaticallyDetectedAdsSubdivisionTargetingCode(
      subdivision_targeting_code);

  return true;
}

void SubdivisionTargeting::Retry() {
  const base::Time time = retry_timer_.StartWithBackoff(
      kRetryFetchSubdivisionTargetingAfterSeconds,
          base::BindOnce(&SubdivisionTargeting::Fetch, base::Unretained(this)));

  BLOG(1, "Retry fetching ads subdivision " << FriendlyDateAndTime(time));
}

void SubdivisionTargeting::FetchAfterDelay() {
  const uint64_t delay = _is_debug ? kDebugFetchSubdivisionTargetingPing :
      kDefaultFetchSubdivisionTargetingPing;

  const base::Time time = timer_.StartWithPrivacy(delay,
      base::BindOnce(&SubdivisionTargeting::Fetch, base::Unretained(this)));

  BLOG(1, "Fetch ads subdivision " << FriendlyDateAndTime(time));
}

void SubdivisionTargeting::BuildUrl() {
  switch (_environment) {
    case Environment::PRODUCTION: {
      url_ = PRODUCTION_SERVER;
      break;
    }

    case Environment::STAGING: {
      url_ = STAGING_SERVER;
      break;
    }

    case Environment::DEVELOPMENT: {
      url_ = DEVELOPMENT_SERVER;
      break;
    }
  }

  url_ += GETSTATE_PATH;
}

}  // namespace ads
