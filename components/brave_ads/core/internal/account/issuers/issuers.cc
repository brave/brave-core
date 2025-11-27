/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers.h"

#include <optional>
#include <utility>

#include "base/check.h"
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

Issuers::Issuers() = default;

Issuers::~Issuers() {
  delegate_ = nullptr;
}

void Issuers::PeriodicallyFetch() {
  if (!is_periodically_fetching_) {
    is_periodically_fetching_ = true;
    Fetch();
  }
}

///////////////////////////////////////////////////////////////////////////////

void Issuers::Fetch() {
  if (is_fetching_ || timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Fetch issuers");

  is_fetching_ = true;

  IssuersUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr mojom_url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(mojom_url_request));
  BLOG(7, UrlRequestHeadersToString(mojom_url_request));

  GetAdsClient().UrlRequest(
      std::move(mojom_url_request),
      base::BindOnce(&Issuers::FetchCallback, weak_factory_.GetWeakPtr()));
}

void Issuers::FetchCallback(const mojom::UrlResponseInfo& mojom_url_response) {
  BLOG(6, UrlResponseToString(mojom_url_response));
  BLOG(7, UrlResponseHeadersToString(mojom_url_response));

  is_fetching_ = false;

  if (mojom_url_response.code == net::HTTP_UPGRADE_REQUIRED) {
    BLOG(0, "Failed to fetch issuers as a browser upgrade is required");
    return AdsNotifierManager::GetInstance()
        .NotifyBrowserUpgradeRequiredToServeAds();
  }

  if (mojom_url_response.code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Failed to request issuers as forbidden");
    return FailedToFetchIssuers(/*should_retry=*/false);
  }

  if (mojom_url_response.code != net::HTTP_OK) {
    return FailedToFetchIssuers(/*should_retry=*/true);
  }

  BLOG(1, "Parsing issuers");
  std::optional<IssuersInfo> issuers =
      json::reader::ReadIssuers(mojom_url_response.body);
  if (!issuers) {
    BLOG(0, "Failed to parse issuers");
    return FailedToFetchIssuers(/*should_retry=*/true);
  }

  SuccessfullyFetchedIssuers(*issuers);
}

void Issuers::SuccessfullyFetchedIssuers(const IssuersInfo& issuers) {
  BLOG(1, "Successfully fetched issuers");

  StopRetrying();

  NotifyDidFetchIssuers(issuers);

  FetchAfterDelay();
}

void Issuers::FailedToFetchIssuers(bool should_retry) {
  BLOG(0, "Failed to fetch issuers");

  NotifyFailedToFetchIssuers();

  if (should_retry) {
    Retry();
  }
}

void Issuers::FetchAfterDelay() {
  CHECK(!timer_.IsRunning());

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, GetFetchDelay(),
      base::BindOnce(&Issuers::Fetch, weak_factory_.GetWeakPtr()));

  BLOG(1, "Fetch issuers " << FriendlyDateAndTime(fetch_at));

  NotifyWillFetchIssuers(fetch_at);
}

void Issuers::Retry() {
  if (timer_.IsRunning()) {
    // The function `WallClockTimer::PowerSuspendObserver::OnResume` restarts
    // the timer to fire at the desired run time after system power is resumed.
    // It's important to note that URL requests might not succeed upon power
    // restoration, triggering a retry. To avoid initiating a second timer, we
    // refrain from starting another one.
    return;
  }

  const base::Time retry_at = timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&Issuers::RetryCallback, weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry fetching issuers " << FriendlyDateAndTime(retry_at));

  NotifyWillRetryFetchingIssuers(retry_at);
}

void Issuers::RetryCallback() {
  BLOG(1, "Retry fetching issuers");

  NotifyDidRetryFetchingIssuers();

  Fetch();
}

void Issuers::StopRetrying() {
  timer_.Stop();
}

void Issuers::NotifyDidFetchIssuers(const IssuersInfo& issuers) const {
  if (delegate_) {
    delegate_->OnDidFetchIssuers(issuers);
  }
}

void Issuers::NotifyFailedToFetchIssuers() const {
  if (delegate_) {
    delegate_->OnFailedToFetchIssuers();
  }
}

void Issuers::NotifyWillFetchIssuers(base::Time fetch_at) const {
  if (delegate_) {
    delegate_->OnWillFetchIssuers(fetch_at);
  }
}

void Issuers::NotifyWillRetryFetchingIssuers(base::Time retry_at) const {
  if (delegate_) {
    delegate_->OnWillRetryFetchingIssuers(retry_at);
  }
}

void Issuers::NotifyDidRetryFetchingIssuers() const {
  if (delegate_) {
    delegate_->OnDidRetryFetchingIssuers();
  }
}

}  // namespace brave_ads
