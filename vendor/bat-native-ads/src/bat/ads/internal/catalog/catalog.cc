/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog.h"

#include <utility>

#include "absl/types/optional.h"
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/catalog/catalog_constants.h"
#include "bat/ads/internal/catalog/catalog_info.h"
#include "bat/ads/internal/catalog/catalog_json_reader.h"
#include "bat/ads/internal/catalog/catalog_url_request_builder.h"
#include "bat/ads/internal/catalog/catalog_util.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time/time_formatting_util.h"
#include "bat/ads/internal/common/url/url_request_string_util.h"
#include "bat/ads/internal/common/url/url_response_string_util.h"
#include "bat/ads/internal/database/database_manager.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "net/http/http_status_code.h"

namespace ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Minutes(1);

constexpr base::TimeDelta kDebugCatalogPing = base::Minutes(15);

}  // namespace

Catalog::Catalog() {
  DatabaseManager::GetInstance()->AddObserver(this);
}

Catalog::~Catalog() {
  DatabaseManager::GetInstance()->RemoveObserver(this);
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
  if (is_processing_ || retry_timer_.IsRunning()) {
    return;
  }

  Fetch();
}

///////////////////////////////////////////////////////////////////////////////

void Catalog::Fetch() {
  DCHECK(!is_processing_);

  BLOG(1, "Catalog");
  BLOG(2, "GET /v" << kCatalogVersion << "/catalog");

  is_processing_ = true;

  CatalogUrlRequestBuilder url_request_builder;
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  AdsClientHelper::GetInstance()->UrlRequest(
      std::move(url_request),
      base::BindOnce(&Catalog::OnFetch, base::Unretained(this)));
}

void Catalog::OnFetch(const mojom::UrlResponseInfo& url_response) {
  BLOG(1, "OnCatalog");

  BLOG(7, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  is_processing_ = false;

  if (url_response.status_code == net::HTTP_NOT_MODIFIED) {
    BLOG(1, "Catalog is up to date");
    FetchAfterDelay();
    return;
  }

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to fetch catalog");
    NotifyFailedToUpdateCatalog();
    Retry();
    return;
  }

  BLOG(1, "Successfully fetched catalog");

  BLOG(1, "Parsing catalog");
  const absl::optional<CatalogInfo> catalog =
      json::reader::ReadCatalog(url_response.body);
  if (!catalog) {
    BLOG(1, "Failed to parse catalog");
    NotifyFailedToUpdateCatalog();
    Retry();
    return;
  }

  if (catalog->version != kCatalogVersion) {
    BLOG(1, "Catalog version mismatch");
    NotifyFailedToUpdateCatalog();
    Retry();
    return;
  }

  SetCatalogLastUpdated(base::Time::Now());

  if (!HasCatalogChanged(catalog->id)) {
    BLOG(1, "Catalog id " << catalog->id << " is up to date");
    FetchAfterDelay();
    return;
  }

  SaveCatalog(*catalog);
  NotifyDidUpdateCatalog(*catalog);
  FetchAfterDelay();
}

void Catalog::FetchAfterDelay() {
  retry_timer_.Stop();

  const base::TimeDelta delay = FlagManager::GetInstance()->ShouldDebug()
                                    ? kDebugCatalogPing
                                    : GetCatalogPing();

  const base::Time fetch_at = timer_.StartWithPrivacy(
      FROM_HERE, delay,
      base::BindOnce(&Catalog::Fetch, base::Unretained(this)));

  BLOG(1, "Fetch catalog " << FriendlyDateAndTime(fetch_at,
                                                  /*use_sentence_style*/ true));
}

void Catalog::Retry() {
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&Catalog::OnRetry, base::Unretained(this)));

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

}  // namespace ads
