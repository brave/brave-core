/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_json_reader.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
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
  const int ping =
      AdsClientHelper::GetInstance()->GetIntegerPref(prefs::kIssuerPing);
  return base::Milliseconds(ping);
}

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
  CHECK(!is_fetching_);

  BLOG(1, "FetchIssuers" << BuildIssuersUrlPath());

  is_fetching_ = true;

  IssuersUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request),
      base::BindOnce(&Issuers::FetchCallback, weak_factory_.GetWeakPtr()));
}

void Issuers::FetchCallback(const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnFetchIssuers");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  is_fetching_ = false;

  if (url_response.status_code != net::HTTP_OK) {
    return FailedToFetchIssuers(/*should_retry*/ true);
  }

  const absl::optional<IssuersInfo> issuers =
      json::reader::ReadIssuers(url_response.body);
  if (!issuers) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    return FailedToFetchIssuers(/*should_retry*/ true);
  }

  SuccessfullyFetchedIssuers(*issuers);
}

void Issuers::SuccessfullyFetchedIssuers(const IssuersInfo& issuers) {
  StopRetrying();

  if (delegate_) {
    delegate_->OnDidFetchIssuers(issuers);
  }

  FetchAfterDelay();
}

void Issuers::FailedToFetchIssuers(const bool should_retry) {
  BLOG(0, "Failed to fetch issuers");

  if (delegate_) {
    delegate_->OnFailedToFetchIssuers();
  }

  if (should_retry) {
    RetryAfterDelay();
  }
}

void Issuers::FetchAfterDelay() {
  CHECK(!retry_timer_.IsRunning());

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, GetFetchDelay(),
      base::BindOnce(&Issuers::Fetch, weak_factory_.GetWeakPtr()));

  BLOG(1, "Fetch issuers " << FriendlyDateAndTime(fetch_at,
                                                  /*use_sentence_style*/ true));
}

void Issuers::RetryAfterDelay() {
  CHECK(!timer_.IsRunning());

  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&Issuers::RetryCallback, weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry fetching issuers "
              << FriendlyDateAndTime(retry_at, /*use_sentence_style*/ true));

  if (delegate_) {
    delegate_->OnWillRetryFetchingIssuers(retry_at);
  }
}

void Issuers::RetryCallback() {
  BLOG(1, "Retry fetching issuers");

  if (delegate_) {
    delegate_->OnDidRetryFetchingIssuers();
  }

  Fetch();
}

void Issuers::StopRetrying() {
  retry_timer_.Stop();
}

}  // namespace brave_ads
