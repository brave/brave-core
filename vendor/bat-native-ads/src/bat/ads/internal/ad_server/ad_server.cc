/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_server/ad_server.h"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include "base/time/time.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/account/confirmations/confirmations.h"
#include "bat/ads/internal/ad_server/get_catalog_url_request_builder.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/bundle/bundle.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/catalog/catalog_issuers_info.h"
#include "bat/ads/internal/catalog/catalog_version.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/server/ads_server_util.h"
#include "bat/ads/internal/time_formatting_util.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {

const int64_t kRetryAfterSeconds = 1 * base::Time::kSecondsPerMinute;

const int64_t kDebugCatalogPing = 15 * base::Time::kSecondsPerMinute;

}  // namespace

AdServer::AdServer() = default;

AdServer::~AdServer() = default;

void AdServer::AddObserver(AdServerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void AdServer::RemoveObserver(AdServerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void AdServer::MaybeFetch() {
  if (is_processing_ || retry_timer_.IsRunning()) {
    return;
  }

  Fetch();
}

///////////////////////////////////////////////////////////////////////////////

void AdServer::Fetch() {
  DCHECK(!is_processing_);

  BLOG(1, "Get catalog");
  BLOG(2, "GET /v" << kCurrentCatalogVersion << "/catalog");

  is_processing_ = true;

  GetCatalogUrlRequestBuilder url_request_builder;
  UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(5, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  auto callback = std::bind(&AdServer::OnFetch, this, std::placeholders::_1);
  AdsClientHelper::Get()->UrlRequest(std::move(url_request), callback);
}

void AdServer::OnFetch(const UrlResponse& url_response) {
  BLOG(7, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  is_processing_ = false;

  if (url_response.status_code / 100 == 2) {
    BLOG(1, "Successfully fetched catalog");

    BLOG(1, "Parsing catalog");

    Catalog catalog;
    if (catalog.FromJson(url_response.body)) {
      SaveCatalog(catalog);

      NotifyCatalogUpdated(catalog);

      FetchAfterDelay();

      return;
    }
  } else if (url_response.status_code == 304) {
    BLOG(1, "Catalog is up to date");

    FetchAfterDelay();

    return;
  }

  BLOG(1, "Failed to parse catalog");

  NotifyCatalogFailed();
  Retry();
}

void AdServer::SaveCatalog(const Catalog& catalog) {
  const std::string last_catalog_id =
      AdsClientHelper::Get()->GetStringPref(prefs::kCatalogId);

  const std::string catalog_id = catalog.GetId();

  if (!catalog.HasChanged(last_catalog_id)) {
    BLOG(1, "Catalog id " << catalog_id << " is up to date");
    return;
  }

  AdsClientHelper::Get()->SetStringPref(prefs::kCatalogId, catalog_id);

  const int catalog_version = catalog.GetVersion();
  AdsClientHelper::Get()->SetIntegerPref(prefs::kCatalogVersion,
                                         catalog_version);

  const int64_t catalog_ping = catalog.GetPing();
  AdsClientHelper::Get()->SetInt64Pref(prefs::kCatalogPing, catalog_ping);

  const int64_t catalog_last_updated =
      static_cast<int64_t>(base::Time::Now().ToDoubleT());
  AdsClientHelper::Get()->SetInt64Pref(prefs::kCatalogLastUpdated,
                                       catalog_last_updated);

  Bundle bundle;
  bundle.BuildFromCatalog(catalog);
}

void AdServer::Retry() {
  const base::Time time = retry_timer_.StartWithPrivacy(
      base::TimeDelta::FromSeconds(kRetryAfterSeconds),
      base::BindOnce(&AdServer::OnRetry, base::Unretained(this)));

  BLOG(1, "Retry fetching catalog " << FriendlyDateAndTime(time));
}

void AdServer::OnRetry() {
  BLOG(1, "Retry fetching catalog");

  Fetch();
}

void AdServer::FetchAfterDelay() {
  retry_timer_.Stop();

  const int64_t ping =
      g_is_debug ? kDebugCatalogPing
                 : AdsClientHelper::Get()->GetInt64Pref(prefs::kCatalogPing);

  const base::TimeDelta delay = base::TimeDelta::FromSeconds(ping);

  const base::Time time = timer_.StartWithPrivacy(
      delay, base::BindOnce(&AdServer::Fetch, base::Unretained(this)));

  BLOG(1, "Fetch catalog " << FriendlyDateAndTime(time));
}

void AdServer::NotifyCatalogUpdated(const Catalog& catalog) const {
  for (AdServerObserver& observer : observers_) {
    observer.OnCatalogUpdated(catalog);
  }
}

void AdServer::NotifyCatalogFailed() const {
  for (AdServerObserver& observer : observers_) {
    observer.OnCatalogFailed();
  }
}

}  // namespace ads
