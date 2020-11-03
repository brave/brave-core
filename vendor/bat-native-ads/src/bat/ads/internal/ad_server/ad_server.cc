/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_server/ad_server.h"

#include <functional>
#include <utility>

#include "bat/ads/internal/account/wallet.h"
#include "bat/ads/internal/ad_server/get_catalog_url_request_builder.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/bundle/bundle.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/server/ads_server_util.h"
#include "bat/ads/internal/time_util.h"
#include "bat/ads/internal/tokens/refill_unblinded_tokens/refill_unblinded_tokens.h"

namespace ads {

using std::placeholders::_1;

namespace {

const uint64_t kRetryAfterSeconds = 1 * base::Time::kSecondsPerMinute;

const uint64_t kDebugCatalogPing = 15 * base::Time::kSecondsPerMinute;

}  // namespace

AdServer::AdServer(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdServer::~AdServer() = default;

void AdServer::MaybeFetch() {
  if (is_processing_ || retry_timer_.IsRunning()) {
    return;
  }

  Fetch();
}

uint64_t AdServer::LastUpdated() const {
  return last_updated_;
}

///////////////////////////////////////////////////////////////////////////////

void AdServer::Fetch() {
  DCHECK(!is_processing_);

  BLOG(1, "Get catalog");
  BLOG(2, "GET /v5/catalog");

  is_processing_ = true;

  GetCatalogUrlRequestBuilder url_request_builder;
  UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(5, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  auto callback = std::bind(&AdServer::OnFetch, this, _1);
  ads_->get_ads_client()->UrlRequest(std::move(url_request), callback);
}

void AdServer::OnFetch(
    const UrlResponse& url_response) {
  BLOG(7, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  is_processing_ = false;

  bool should_retry = false;

  if (url_response.status_code / 100 == 2) {
    if (!url_response.body.empty()) {
      BLOG(1, "Successfully fetched catalog");
    }

    if (!Parse(url_response.body)) {
      should_retry = true;
    }
  } else if (url_response.status_code == 304) {
    BLOG(1, "Catalog is up to date");
  } else {
    BLOG(1, "Failed to fetch catalog");

    should_retry = true;
  }

  const WalletInfo wallet = ads_->get_wallet()->Get();
  ads_->get_refill_unblinded_tokens()->MaybeRefill(wallet);

  ads_->get_confirmations()->RetryFailedConfirmationsAfterDelay();

  if (should_retry) {
    Retry();
    return;
  }

  retry_timer_.Stop();

  FetchAfterDelay();
}

bool AdServer::Parse(
    const std::string& json) {
  BLOG(1, "Parsing catalog");

  Catalog catalog(ads_);
  if (!catalog.FromJson(json)) {
    BLOG(0, "Failed to load catalog");

    BLOG(3, "Failed to parse catalog: " << json);

    return false;
  }

  if (!catalog.HasChanged(ads_->get_bundle()->GetCatalogId())) {
    BLOG(1, "Catalog id " << catalog.GetId() << " matches current catalog id "
        << ads_->get_bundle()->GetCatalogId());

    return true;
  }

  BLOG(1, "Generating bundle");

  if (!ads_->get_bundle()->UpdateFromCatalog(catalog)) {
    BLOG(0, "Failed to generate bundle");

    return false;
  }

  BLOG(1, "Successfully generated bundle");

  auto callback = std::bind(&AdServer::OnSaved, this, _1);
  catalog.Save(json, callback);

  CatalogIssuersInfo catalog_issuers = catalog.GetIssuers();
  ads_->get_confirmations()->SetCatalogIssuers(catalog_issuers);

  return true;
}

void AdServer::OnSaved(
    const Result result) {
  if (result != SUCCESS) {
    // If the catalog fails to save, we will retry the next time we fetch the
    // catalog

    BLOG(0, "Failed to save catalog");

    return;
  }

  BLOG(3, "Successfully saved catalog");
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
  const uint64_t ping = _is_debug ? kDebugCatalogPing :
      ads_->get_bundle()->GetCatalogPing();

  const base::TimeDelta delay = base::TimeDelta::FromSeconds(ping);

  const base::Time time = timer_.StartWithPrivacy(delay,
      base::BindOnce(&AdServer::Fetch, base::Unretained(this)));

  BLOG(1, "Fetch catalog " << FriendlyDateAndTime(time));
}

}  // namespace ads
