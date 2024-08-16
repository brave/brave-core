/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request.h"

#include <optional>
#include <utility>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_feature.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_json_reader_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_flag_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kDebugFetchAfter = base::Minutes(5);

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);

}  // namespace

SubdivisionUrlRequest::SubdivisionUrlRequest() = default;

SubdivisionUrlRequest::~SubdivisionUrlRequest() {
  delegate_ = nullptr;
}

void SubdivisionUrlRequest::PeriodicallyFetch() {
  if (is_periodically_fetching_) {
    return;
  }

  is_periodically_fetching_ = true;

  Fetch();
}

///////////////////////////////////////////////////////////////////////////////

void SubdivisionUrlRequest::Fetch() {
  if (is_fetching_ || timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Fetch subdivision");

  is_fetching_ = true;

  GetSubdivisionUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  GetAdsClient()->UrlRequest(
      std::move(url_request),
      base::BindOnce(&SubdivisionUrlRequest::FetchCallback,
                     weak_factory_.GetWeakPtr()));
}

void SubdivisionUrlRequest::FetchCallback(
    const mojom::UrlResponseInfo& url_response) {
  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  is_fetching_ = false;

  if (url_response.status_code != net::HTTP_OK) {
    return FailedToFetchSubdivision();
  }

  BLOG(1, "Parsing subdivision");
  const std::optional<std::string> subdivision =
      json::reader::ParseSubdivision(url_response.body);
  if (!subdivision) {
    BLOG(0, "Failed to parse subdivision");
    return FailedToFetchSubdivision();
  }

  SuccessfullyFetchedSubdivision(*subdivision);
}

void SubdivisionUrlRequest::FetchAfterDelay() {
  CHECK(!timer_.IsRunning());

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE,
      ShouldDebug() ? kDebugFetchAfter : kFetchSubdivisionAfter.Get(),
      base::BindOnce(&SubdivisionUrlRequest::Fetch,
                     weak_factory_.GetWeakPtr()));

  BLOG(1, "Fetch subdivision " << FriendlyDateAndTime(fetch_at));

  NotifyWillFetchSubdivision(fetch_at);
}

void SubdivisionUrlRequest::SuccessfullyFetchedSubdivision(
    const std::string& subdivision) {
  BLOG(1, "Successfully fetched subdivision");

  StopRetrying();

  NotifyDidFetchSubdivision(subdivision);

  FetchAfterDelay();
}

void SubdivisionUrlRequest::FailedToFetchSubdivision() {
  BLOG(1, "Failed to fetch subdivision");

  NotifyFailedToFetchSubdivision();

  Retry();
}

void SubdivisionUrlRequest::Retry() {
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
      base::BindOnce(&SubdivisionUrlRequest::RetryCallback,
                     weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry fetching subdivision " << FriendlyDateAndTime(retry_at));

  NotifyWillRetryFetchingSubdivision(retry_at);
}

void SubdivisionUrlRequest::RetryCallback() {
  BLOG(1, "Retry fetching subdivision");

  NotifyDidRetryFetchingSubdivision();

  Fetch();
}

void SubdivisionUrlRequest::StopRetrying() {
  timer_.Stop();
}

void SubdivisionUrlRequest::NotifyWillFetchSubdivision(
    const base::Time fetch_at) const {
  if (delegate_) {
    delegate_->OnWillFetchSubdivision(fetch_at);
  }
}

void SubdivisionUrlRequest::NotifyDidFetchSubdivision(
    const std::string& subdivision) const {
  if (delegate_) {
    delegate_->OnDidFetchSubdivision(subdivision);
  }
}

void SubdivisionUrlRequest::NotifyFailedToFetchSubdivision() const {
  if (delegate_) {
    delegate_->OnFailedToFetchSubdivision();
  }
}

void SubdivisionUrlRequest::NotifyWillRetryFetchingSubdivision(
    const base::Time retry_at) const {
  if (delegate_) {
    delegate_->OnWillRetryFetchingSubdivision(retry_at);
  }
}

void SubdivisionUrlRequest::NotifyDidRetryFetchingSubdivision() const {
  if (delegate_) {
    delegate_->OnDidRetryFetchingSubdivision();
  }
}

}  // namespace brave_ads
