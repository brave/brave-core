/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers.h"

#include <functional>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/time/time.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_json_reader.h"
#include "bat/ads/internal/account/issuers/issuers_url_request_builder.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/base/net/http/http_status_code.h"
#include "bat/ads/internal/base/time/time_formatting_util.h"
#include "bat/ads/internal/base/url/url_request_string_util.h"
#include "bat/ads/internal/base/url/url_response_string_util.h"
#include "bat/ads/pref_names.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

namespace {
constexpr base::TimeDelta kRetryAfter = base::Minutes(1);
}  // namespace

Issuers::Issuers() = default;

Issuers::~Issuers() {
  delegate_ = nullptr;
}

void Issuers::MaybeFetch() {
  if (is_fetching_ || retry_timer_.IsRunning()) {
    return;
  }

  Fetch();
}

//////////////////////////////////////////////////////////////////////////////

void Issuers::Fetch() {
  DCHECK(!is_fetching_);

  is_fetching_ = true;

  BLOG(1, "FetchIssuers");
  BLOG(2, "GET /v1/issuers/");

  IssuersUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request),
      base::BindOnce(&Issuers::OnFetch, base::Unretained(this)));
}

void Issuers::OnFetch(const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnFetchIssuers");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    FailedToFetchIssuers(/*should_retry*/ true);
    return;
  }

  const absl::optional<IssuersInfo> issuers =
      JSONReader::ReadIssuers(url_response.body);
  if (!issuers) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    FailedToFetchIssuers(/*should_retry*/ true);
    return;
  }

  SuccessfullyFetchedIssuers(*issuers);
}

void Issuers::SuccessfullyFetchedIssuers(const IssuersInfo& issuers) {
  StopRetrying();

  is_fetching_ = false;

  if (delegate_) {
    delegate_->OnDidFetchIssuers(issuers);
  }

  FetchAfterDelay();
}

void Issuers::FailedToFetchIssuers(const bool should_retry) {
  BLOG(0, "Failed to fetch issuers");

  is_fetching_ = false;

  if (delegate_) {
    delegate_->OnFailedToFetchIssuers();
  }

  if (should_retry) {
    RetryAfterDelay();
  }
}

void Issuers::FetchAfterDelay() {
  DCHECK(!retry_timer_.IsRunning());

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, GetFetchDelay(),
      base::BindOnce(&Issuers::Fetch, base::Unretained(this)));

  BLOG(1, "Fetch issuers " << FriendlyDateAndTime(fetch_at,
                                                  /*use_sentence_style*/ true));
}

base::TimeDelta Issuers::GetFetchDelay() const {
  const int ping =
      AdsClientHelper::GetInstance()->GetIntegerPref(prefs::kIssuerPing);
  return base::Milliseconds(ping);
}

void Issuers::RetryAfterDelay() {
  DCHECK(!timer_.IsRunning());

  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&Issuers::OnRetry, base::Unretained(this)));

  BLOG(1, "Retry fetching issuers "
              << FriendlyDateAndTime(retry_at, /*use_sentence_style*/ true));

  if (delegate_) {
    delegate_->OnWillRetryFetchingIssuers(retry_at);
  }
}

void Issuers::OnRetry() {
  BLOG(1, "Retry fetching issuers");

  if (delegate_) {
    delegate_->OnDidRetryFetchingIssuers();
  }

  Fetch();
}

void Issuers::StopRetrying() {
  retry_timer_.Stop();
}

}  // namespace ads
