/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog.h"

#include <utility>

#include "base/check.h"
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
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_flag_util.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);

constexpr base::TimeDelta kDebugCatalogPing = base::Minutes(15);

}  // namespace

Catalog::Catalog() {
  DatabaseManager::GetInstance().AddObserver(this);
}

Catalog::~Catalog() {
  DatabaseManager::GetInstance().RemoveObserver(this);
}

void Catalog::AddObserver(CatalogObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Catalog::RemoveObserver(CatalogObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void Catalog::MaybeFetch() {
  if (is_fetching_ || retry_timer_.IsRunning()) {
    return;
  }

  Fetch();
}

///////////////////////////////////////////////////////////////////////////////

void Catalog::Fetch() {
  DCHECK(!is_fetching_);

  BLOG(1, "FetchCatalog " << BuildCatalogUrlPath());

  is_fetching_ = true;

  CatalogUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request),
      base::BindOnce(&Catalog::OnFetch, weak_factory_.GetWeakPtr()));
}

void Catalog::OnFetch(const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnFetchCatalog");

  BLOG(7, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  is_fetching_ = false;

  if (url_response.status_code == net::HTTP_NOT_MODIFIED) {
    BLOG(1, "Catalog is up to date");
    return FetchAfterDelay();
  }

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to fetch catalog");
    NotifyFailedToUpdateCatalog();
    return Retry();
  }

  BLOG(1, "Successfully fetched catalog");

  BLOG(1, "Parsing catalog");
  const absl::optional<CatalogInfo> catalog =
      json::reader::ReadCatalog(url_response.body);
  if (!catalog) {
    BLOG(1, "Failed to parse catalog");
    NotifyFailedToUpdateCatalog();
    return Retry();
  }

  if (catalog->version != kCatalogVersion) {
    BLOG(1, "Catalog version mismatch");
    NotifyFailedToUpdateCatalog();
    return Retry();
  }

  SetCatalogLastUpdated(base::Time::Now());

  if (!HasCatalogChanged(catalog->id)) {
    BLOG(1, "Catalog id " << catalog->id << " is up to date");
    return FetchAfterDelay();
  }

  SaveCatalog(*catalog);
  NotifyDidUpdateCatalog(*catalog);
  FetchAfterDelay();
}

void Catalog::FetchAfterDelay() {
  retry_timer_.Stop();

  const base::TimeDelta delay =
      ShouldDebug() ? kDebugCatalogPing : GetCatalogPing();

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, delay,
      base::BindOnce(&Catalog::Fetch, weak_factory_.GetWeakPtr()));

  BLOG(1, "Fetch catalog " << FriendlyDateAndTime(fetch_at,
                                                  /*use_sentence_style*/ true));
}

void Catalog::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&Catalog::OnRetry, weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry fetching catalog "
              << FriendlyDateAndTime(retry_at, /*use_sentence_style*/ true));
}

void Catalog::OnRetry() {
  BLOG(1, "Retry fetching catalog");

  Fetch();
}

void Catalog::NotifyDidUpdateCatalog(const CatalogInfo& catalog) const {
  for (CatalogObserver& observer : observers_) {
    observer.OnDidUpdateCatalog(catalog);
  }
}

void Catalog::NotifyFailedToUpdateCatalog() const {
  for (CatalogObserver& observer : observers_) {
    observer.OnFailedToUpdateCatalog();
  }
}

void Catalog::OnDidMigrateDatabase(const int /*from_version*/,
                                   const int /*to_version*/) {
  ResetCatalog();
}

}  // namespace brave_ads
