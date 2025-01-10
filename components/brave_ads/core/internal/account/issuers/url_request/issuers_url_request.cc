/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_json_reader.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/ads_notifier_manager.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);

base::TimeDelta GetFetchDelay() {
  return base::Milliseconds(GetProfileIntegerPref(prefs::kIssuerPing));
}

}  // namespace

IssuersUrlRequest::IssuersUrlRequest() = default;

IssuersUrlRequest::~IssuersUrlRequest() {
  delegate_ = nullptr;
}

void IssuersUrlRequest::PeriodicallyFetch() {
  if (!is_periodically_fetching_) {
    is_periodically_fetching_ = true;
    Fetch();
  }
}

///////////////////////////////////////////////////////////////////////////////

void IssuersUrlRequest::Fetch() {
  if (is_fetching_ || timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Fetch issuers");

  is_fetching_ = true;

  IssuersUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr mojom_url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(mojom_url_request));
  BLOG(7, UrlRequestHeadersToString(mojom_url_request));

  GetAdsClient().UrlRequest(std::move(mojom_url_request),
                            base::BindOnce(&IssuersUrlRequest::FetchCallback,
                                           weak_factory_.GetWeakPtr()));
}

void IssuersUrlRequest::FetchCallback(
    const mojom::UrlResponseInfo& mojom_url_response) {
  BLOG(6, UrlResponseToString(mojom_url_response));
  BLOG(7, UrlResponseHeadersToString(mojom_url_response));

  is_fetching_ = false;

  if (mojom_url_response.status_code == net::HTTP_UPGRADE_REQUIRED) {
    BLOG(1, "Failed to fetch issuers as a browser upgrade is required");
    return AdsNotifierManager::GetInstance()
        .NotifyBrowserUpgradeRequiredToServeAds();
  }

  if (mojom_url_response.status_code != net::HTTP_OK) {
    return FailedToFetchIssuers();
  }

  BLOG(1, "Parsing issuers");
  const std::optional<IssuersInfo> issuers =
      json::reader::ReadIssuers(mojom_url_response.body);
  if (!issuers) {
    BLOG(0, "Failed to parse issuers");
    return FailedToFetchIssuers();
  }

  SuccessfullyFetchedIssuers(*issuers);
}

void IssuersUrlRequest::SuccessfullyFetchedIssuers(const IssuersInfo& issuers) {
  BLOG(1, "Successfully fetched issuers");

  StopRetrying();

  NotifyDidFetchIssuers(issuers);

  FetchAfterDelay();
}

void IssuersUrlRequest::FailedToFetchIssuers() {
  BLOG(1, "Failed to fetch issuers");

  NotifyFailedToFetchIssuers();

  Retry();
}

void IssuersUrlRequest::FetchAfterDelay() {
  CHECK(!timer_.IsRunning());

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, GetFetchDelay(),
      base::BindOnce(&IssuersUrlRequest::Fetch, weak_factory_.GetWeakPtr()));

  BLOG(1, "Fetch issuers " << FriendlyDateAndTime(fetch_at));

  NotifyWillFetchIssuers(fetch_at);
}

void IssuersUrlRequest::Retry() {
  if (timer_.IsRunning()) {
    // The function `WallClockTimer::PowerSuspendObserver::OnResume` restarts
    // the timer to fire at the desired run time after system power is resumed.
    // It's important to note that URL requests might not succeed upon power
    // restoration, triggering a retry. To avoid initiating a second timer, we
    // refrain from starting another one.
    return;
  }

  const base::Time retry_at =
      timer_.StartWithPrivacy(FROM_HERE, kRetryAfter,
                              base::BindOnce(&IssuersUrlRequest::RetryCallback,
                                             weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry fetching issuers " << FriendlyDateAndTime(retry_at));

  NotifyWillRetryFetchingIssuers(retry_at);
}

void IssuersUrlRequest::RetryCallback() {
  BLOG(1, "Retry fetching issuers");

  NotifyDidRetryFetchingIssuers();

  Fetch();
}

void IssuersUrlRequest::StopRetrying() {
  timer_.Stop();
}

void IssuersUrlRequest::NotifyDidFetchIssuers(
    const IssuersInfo& issuers) const {
  if (delegate_) {
    delegate_->OnDidFetchIssuers(issuers);
  }
}

void IssuersUrlRequest::NotifyFailedToFetchIssuers() const {
  if (delegate_) {
    delegate_->OnFailedToFetchIssuers();
  }
}

void IssuersUrlRequest::NotifyWillFetchIssuers(base::Time fetch_at) const {
  if (delegate_) {
    delegate_->OnWillFetchIssuers(fetch_at);
  }
}

void IssuersUrlRequest::NotifyWillRetryFetchingIssuers(
    base::Time retry_at) const {
  if (delegate_) {
    delegate_->OnWillRetryFetchingIssuers(retry_at);
  }
}

void IssuersUrlRequest::NotifyDidRetryFetchingIssuers() const {
  if (delegate_) {
    delegate_->OnDidRetryFetchingIssuers();
  }
}

}  // namespace brave_ads
