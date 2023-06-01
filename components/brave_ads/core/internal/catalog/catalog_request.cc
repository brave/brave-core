/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_request.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_json_reader.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_flag_util.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);

constexpr base::TimeDelta kDebugCatalogPing = base::Minutes(15);

}  // namespace

CatalogRequest::CatalogRequest() = default;

CatalogRequest::~CatalogRequest() {
  delegate_ = nullptr;
}

void CatalogRequest::PeriodicallyFetch() {
  if (is_periodically_fetching_) {
    return;
  }

  is_periodically_fetching_ = true;

  Fetch();
}

///////////////////////////////////////////////////////////////////////////////

void CatalogRequest::Fetch() {
  if (is_fetching_ || retry_timer_.IsRunning()) {
    return;
  }

  BLOG(1, "FetchCatalog " << BuildCatalogUrlPath());

  is_fetching_ = true;

  CatalogUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request), base::BindOnce(&CatalogRequest::FetchCallback,
                                             weak_factory_.GetWeakPtr()));
}

void CatalogRequest::FetchCallback(const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnFetchCatalog");

  BLOG(7, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  is_fetching_ = false;

  if (url_response.status_code == net::HTTP_NOT_MODIFIED) {
    BLOG(1, "Catalog is up to date");
    return FetchAfterDelay();
  }

  if (url_response.status_code != net::HTTP_OK) {
    if (delegate_) {
      delegate_->OnFailedToFetchCatalog();
    }

    return Retry();
  }

  BLOG(1, "Parsing catalog");
  const absl::optional<CatalogInfo> catalog =
      json::reader::ReadCatalog(url_response.body);
  if (!catalog) {
    BLOG(1, "Failed to parse catalog");

    if (delegate_) {
      delegate_->OnFailedToFetchCatalog();
    }

    return Retry();
  }

  if (catalog->version != kCatalogVersion) {
    BLOG(1, "Catalog version mismatch");

    if (delegate_) {
      delegate_->OnFailedToFetchCatalog();
    }

    return Retry();
  }

  if (delegate_) {
    delegate_->OnDidFetchCatalog(*catalog);
  }

  FetchAfterDelay();
}

void CatalogRequest::FetchAfterDelay() {
  StopRetrying();

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, ShouldDebug() ? kDebugCatalogPing : GetCatalogPing(),
      base::BindOnce(&CatalogRequest::Fetch, weak_factory_.GetWeakPtr()));

  BLOG(1, "Fetch catalog " << FriendlyDateAndTime(fetch_at));
}

void CatalogRequest::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&CatalogRequest::RetryCallback,
                     weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry fetching catalog " << FriendlyDateAndTime(retry_at));
}

void CatalogRequest::RetryCallback() {
  BLOG(1, "Retry fetching catalog");

  Fetch();
}

void CatalogRequest::StopRetrying() {
  retry_timer_.Stop();
}

}  // namespace brave_ads
