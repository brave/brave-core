/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/get_catalog/get_catalog.h"

#include <functional>
#include <utility>

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/bundle/bundle.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/server/ads_server_util.h"
#include "bat/ads/internal/server/get_catalog/get_catalog_url_request_builder.h"
#include "bat/ads/internal/server/refill_unblinded_tokens/refill_unblinded_tokens.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

using std::placeholders::_1;

namespace {

const uint64_t kRetryAfterSeconds = 1 * base::Time::kSecondsPerMinute;

const uint64_t kDebugCatalogPing = 15 * base::Time::kSecondsPerMinute;

}  // namespace

GetCatalog::GetCatalog(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

GetCatalog::~GetCatalog() = default;

void GetCatalog::Download() {
  if (retry_timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Download catalog");
  BLOG(2, "GET /v3/catalog");

  GetCatalogUrlRequestBuilder url_request_builder;
  UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(5, UrlRequestToString(url_request));

  auto callback = std::bind(&GetCatalog::OnDownloaded, this, _1);
  ads_->get_ads_client()->UrlRequest(std::move(url_request), callback);
}

uint64_t GetCatalog::LastUpdated() const {
  return last_updated_;
}

///////////////////////////////////////////////////////////////////////////////

void GetCatalog::OnDownloaded(
    const UrlResponse& response) {
  BLOG(7, UrlResponseToString(response));

  bool should_retry = false;

  if (response.status_code / 100 == 2) {
    if (!response.body.empty()) {
      BLOG(1, "Successfully downloaded catalog");
    }

    if (!Parse(response.body)) {
      should_retry = true;
    }
  } else if (response.status_code == 304) {
    BLOG(1, "Catalog is up to date");
  } else {
    BLOG(1, "Failed to download catalog");

    should_retry = true;
  }

  if (should_retry) {
    Retry();
    return;
  }

  retry_timer_.Stop();

  DownloadAfterDelay();
}

bool GetCatalog::Parse(
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

  auto callback = std::bind(&GetCatalog::OnSaved, this, _1);
  catalog.Save(json, callback);

  CatalogIssuersInfo catalog_issuers = catalog.GetIssuers();
  ads_->get_confirmations()->SetCatalogIssuers(catalog_issuers);

  ads_->get_refill_unblinded_tokens()->MaybeRefill();
  ads_->get_confirmations()->RetryFailedConfirmationsAfterDelay();

  return true;
}

void GetCatalog::OnSaved(
    const Result result) {
  if (result != SUCCESS) {
    // If the catalog fails to save, we will retry the next time we download the
    // catalog

    BLOG(0, "Failed to save catalog");

    return;
  }

  BLOG(3, "Successfully saved catalog");
}

void GetCatalog::Retry() {
  const base::Time time = retry_timer_.StartWithPrivacy(
      base::TimeDelta::FromSeconds(kRetryAfterSeconds),
          base::BindOnce(&GetCatalog::Download, base::Unretained(this)));

  BLOG(1, "Retry downloading catalog " << FriendlyDateAndTime(time));
}

void GetCatalog::DownloadAfterDelay() {
  const uint64_t ping = _is_debug ? kDebugCatalogPing :
      ads_->get_bundle()->GetCatalogPing();

  const base::TimeDelta delay = base::TimeDelta::FromSeconds(ping);

  const base::Time time = timer_.StartWithPrivacy(delay,
      base::BindOnce(&GetCatalog::Download, base::Unretained(this)));

  BLOG(1, "Download catalog " << FriendlyDateAndTime(time));
}

}  // namespace ads
