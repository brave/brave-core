/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/ads_notifier_manager.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_info.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_json_reader.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/net/http/http_status_code.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_flag_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "net/http/http_status_code.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kDebugCatalogPing = base::Minutes(3);

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);

}  // namespace

CatalogUrlRequest::CatalogUrlRequest() = default;

CatalogUrlRequest::~CatalogUrlRequest() {
  delegate_ = nullptr;
}

void CatalogUrlRequest::PeriodicallyFetch() {
  if (is_periodically_fetching_) {
    return;
  }

  is_periodically_fetching_ = true;

  Fetch();
}

///////////////////////////////////////////////////////////////////////////////

void CatalogUrlRequest::Fetch() {
  if (is_fetching_ || timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Fetch catalog");

  is_fetching_ = true;

  CatalogUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  UrlRequest(std::move(url_request),
             base::BindOnce(&CatalogUrlRequest::FetchCallback,
                            weak_factory_.GetWeakPtr()));
}

void CatalogUrlRequest::FetchCallback(
    const mojom::UrlResponseInfo& url_response) {
  BLOG(7, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  is_fetching_ = false;

  if (url_response.status_code == net::kHttpUpgradeRequired) {
    BLOG(1, "Failed to request catalog as a browser upgrade is required");
    return AdsNotifierManager::GetInstance()
        .NotifyBrowserUpgradeRequiredToServeAds();
  }

  if (url_response.status_code == net::HTTP_NOT_MODIFIED) {
    BLOG(1, "Catalog is up to date");
    return FetchAfterDelay();
  }

  if (url_response.status_code != net::HTTP_OK) {
    return FailedToFetchCatalog();
  }

  BLOG(1, "Parsing catalog");
  const std::optional<CatalogInfo> catalog =
      json::reader::ReadCatalog(url_response.body);
  if (!catalog) {
    BLOG(0, "Failed to parse catalog");
    return FailedToFetchCatalog();
  }

  if (catalog->version != kCatalogVersion) {
    BLOG(1, "Catalog version mismatch");
    return FailedToFetchCatalog();
  }

  SuccessfullyFetchedCatalog(*catalog);
}

void CatalogUrlRequest::FetchAfterDelay() {
  CHECK(!timer_.IsRunning());

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, ShouldDebug() ? kDebugCatalogPing : GetCatalogPing(),
      base::BindOnce(&CatalogUrlRequest::Fetch, weak_factory_.GetWeakPtr()));

  BLOG(1, "Fetch catalog " << FriendlyDateAndTime(fetch_at));

  NotifyWillFetchCatalog(fetch_at);
}

void CatalogUrlRequest::SuccessfullyFetchedCatalog(const CatalogInfo& catalog) {
  BLOG(1, "Successfully fetched catalog");

  StopRetrying();

  NotifyDidFetchCatalog(catalog);

  FetchAfterDelay();
}

void CatalogUrlRequest::FailedToFetchCatalog() {
  BLOG(1, "Failed to fetch catalog");

  NotifyFailedToFetchCatalog();

  Retry();
}

void CatalogUrlRequest::Retry() {
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
                              base::BindOnce(&CatalogUrlRequest::RetryCallback,
                                             weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry fetching catalog " << FriendlyDateAndTime(retry_at));

  NotifyWillRetryFetchingCatalog(retry_at);
}

void CatalogUrlRequest::RetryCallback() {
  BLOG(1, "Retry fetching catalog");

  NotifyDidRetryFetchingCatalog();

  Fetch();
}

void CatalogUrlRequest::StopRetrying() {
  timer_.Stop();
}

void CatalogUrlRequest::NotifyWillFetchCatalog(
    const base::Time fetch_at) const {
  if (delegate_) {
    delegate_->OnWillFetchCatalog(fetch_at);
  }
}

void CatalogUrlRequest::NotifyDidFetchCatalog(
    const CatalogInfo& catalog) const {
  if (delegate_) {
    delegate_->OnDidFetchCatalog(catalog);
  }
}

void CatalogUrlRequest::NotifyFailedToFetchCatalog() const {
  if (delegate_) {
    delegate_->OnFailedToFetchCatalog();
  }
}

void CatalogUrlRequest::NotifyWillRetryFetchingCatalog(
    const base::Time retry_at) const {
  if (delegate_) {
    delegate_->OnWillRetryFetchingCatalog(retry_at);
  }
}

void CatalogUrlRequest::NotifyDidRetryFetchingCatalog() const {
  if (delegate_) {
    delegate_->OnDidRetryFetchingCatalog();
  }
}

}  // namespace brave_ads
