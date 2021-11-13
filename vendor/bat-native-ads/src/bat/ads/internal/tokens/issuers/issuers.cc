/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/issuers/issuers.h"

#include <cstdint>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/check.h"
#include "base/time/time.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/logging_util.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "bat/ads/internal/tokens/issuers/issuers_info.h"
#include "bat/ads/internal/tokens/issuers/issuers_json_reader.h"
#include "bat/ads/internal/tokens/issuers/issuers_url_request_builder.h"
#include "bat/ads/pref_names.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

namespace {

const int64_t kRetryAfterSeconds = 1 * base::Time::kSecondsPerMinute;

absl::optional<IssuersInfo> ParseJson(const std::string& json) {
  const absl::optional<IssuersInfo>& issuers_optional =
      JSONReader::ReadIssuers(json);
  if (!issuers_optional) {
    return absl::nullopt;
  }

  return issuers_optional;
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
  DCHECK(!is_fetching_);

  is_fetching_ = true;

  BLOG(1, "GetIssuers");
  BLOG(2, "GET /v1/issuers/");

  IssuersUrlRequestBuilder url_request_builder;
  mojom::UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  const auto callback =
      std::bind(&Issuers::OnFetch, this, std::placeholders::_1);
  AdsClientHelper::Get()->UrlRequest(std::move(url_request), callback);
}

void Issuers::OnFetch(const mojom::UrlResponse& url_response) {
  BLOG(1, "OnGetIssuers");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code != net::HTTP_OK) {
    OnFailedToGetIssuers();
    return;
  }

  const absl::optional<IssuersInfo>& issuers_optional =
      ParseJson(url_response.body);
  if (!issuers_optional) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    OnFailedToGetIssuers();
    return;
  }

  const IssuersInfo& issuers = issuers_optional.value();

  OnDidGetIssuers(issuers);
}

void Issuers::OnDidGetIssuers(const IssuersInfo& issuers) {
  StopRetrying();

  is_fetching_ = false;

  if (delegate_) {
    delegate_->OnDidGetIssuers(issuers);
  }

  FetchAfterDelay();
}

void Issuers::OnFailedToGetIssuers() {
  is_fetching_ = false;

  if (delegate_) {
    delegate_->OnFailedToGetIssuers();
  }

  Retry();
}

void Issuers::FetchAfterDelay() {
  DCHECK(!retry_timer_.IsRunning());

  const base::Time& time = timer_.StartWithPrivacy(
      GetFetchDelay(), base::BindOnce(&Issuers::Fetch, base::Unretained(this)));

  BLOG(1, "Fetch issuers " << FriendlyDateAndTime(time));
}

base::TimeDelta Issuers::GetFetchDelay() const {
  const int ping = AdsClientHelper::Get()->GetIntegerPref(prefs::kIssuerPing);
  return base::TimeDelta::FromMilliseconds(ping);
}

void Issuers::Retry() {
  DCHECK(!timer_.IsRunning());

  const base::Time& time = retry_timer_.StartWithPrivacy(
      base::TimeDelta::FromSeconds(kRetryAfterSeconds),
      base::BindOnce(&Issuers::OnRetry, base::Unretained(this)));

  BLOG(1, "Retry fetching issuers " << FriendlyDateAndTime(time));
}

void Issuers::OnRetry() {
  BLOG(1, "Retry fetching issuers");

  Fetch();
}

void Issuers::StopRetrying() {
  retry_timer_.Stop();
}

}  // namespace ads
