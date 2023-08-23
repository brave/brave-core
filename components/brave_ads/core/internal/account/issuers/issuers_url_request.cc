/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_json_reader.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);

base::TimeDelta GetFetchDelay() {
  return base::Milliseconds(
      AdsClientHelper::GetInstance()->GetIntegerPref(prefs::kIssuerPing));
}

}  // namespace

IssuersUrlRequest::IssuersUrlRequest() = default;

IssuersUrlRequest::~IssuersUrlRequest() {
  delegate_ = nullptr;
}

void IssuersUrlRequest::PeriodicallyFetch() {
  if (is_periodically_fetching_) {
    return;
  }

  is_periodically_fetching_ = true;

  Fetch();
}

///////////////////////////////////////////////////////////////////////////////

void IssuersUrlRequest::Fetch() {
  if (is_fetching_ || retry_timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Fetching issuers " << BuildIssuersUrlPath());

  is_fetching_ = true;

  IssuersUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request), base::BindOnce(&IssuersUrlRequest::FetchCallback,
                                             weak_factory_.GetWeakPtr()));
}

void IssuersUrlRequest::FetchCallback(
    const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "Fetched issuers");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  is_fetching_ = false;

  if (url_response.status_code != net::HTTP_OK) {
    return FailedToFetchIssuers();
  }

  BLOG(1, "Parsing issuers");
  const absl::optional<IssuersInfo> issuers =
      json::reader::ReadIssuers(url_response.body);
  if (!issuers) {
    BLOG(3, "Failed to parse issuers");
    return FailedToFetchIssuers();
  }

  SuccessfullyFetchedIssuers(*issuers);
}

void IssuersUrlRequest::SuccessfullyFetchedIssuers(const IssuersInfo& issuers) {
  StopRetrying();

  BLOG(1, "Successfully fetched issuers");

  if (delegate_) {
    delegate_->OnDidFetchIssuers(issuers);
  }

  FetchAfterDelay();
}

void IssuersUrlRequest::FailedToFetchIssuers() {
  BLOG(1, "Failed to fetch issuers");

  if (delegate_) {
    delegate_->OnFailedToFetchIssuers();
  }

  Retry();
}

void IssuersUrlRequest::FetchAfterDelay() {
  CHECK(!retry_timer_.IsRunning());

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, GetFetchDelay(),
      base::BindOnce(&IssuersUrlRequest::Fetch, weak_factory_.GetWeakPtr()));

  BLOG(1, "Fetch issuers " << FriendlyDateAndTime(fetch_at));

  if (delegate_) {
    delegate_->OnWillFetchIssuers(fetch_at);
  }
}

void IssuersUrlRequest::Retry() {
  CHECK(!timer_.IsRunning());

  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&IssuersUrlRequest::RetryCallback,
                     weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry fetching issuers " << FriendlyDateAndTime(retry_at));

  if (delegate_) {
    delegate_->OnWillRetryFetchingIssuers(retry_at);
  }
}

void IssuersUrlRequest::RetryCallback() {
  BLOG(1, "Retry fetching issuers");

  if (delegate_) {
    delegate_->OnDidRetryFetchingIssuers();
  }

  Fetch();
}

void IssuersUrlRequest::StopRetrying() {
  retry_timer_.Stop();
}

}  // namespace brave_ads
