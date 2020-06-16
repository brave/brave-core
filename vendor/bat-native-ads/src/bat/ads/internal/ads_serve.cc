/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ads/internal/ads_serve.h"
#include "bat/ads/internal/static_values.h"
#include "bat/ads/internal/bundle.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_util.h"

#include "base/time/time.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ads {

AdsServe::AdsServe(
    AdsImpl* ads,
    Bundle* bundle)
    : catalog_last_updated_(0),
      ads_(ads),
      bundle_(bundle) {
  BuildUrl();
}

AdsServe::~AdsServe() = default;

void AdsServe::DownloadCatalog() {
  if (retry_timer_.IsRunning()) {
    return;
  }

  BLOG(1, "Download catalog");
  BLOG(2, "GET /v2/catalog");

  auto callback = std::bind(&AdsServe::OnCatalogDownloaded,
      this, url_, _1, _2, _3);

  BLOG(5, UrlRequestToString(url_, {}, "", "", URLRequestMethod::GET));
  ads_->get_ads_client()->URLRequest(url_, {}, "", "", URLRequestMethod::GET,
      callback);
}

void AdsServe::DownloadCatalogAfterDelay() {
  const uint64_t delay = _is_debug ? kDebugCatalogPing :
      bundle_->GetCatalogPing();

  const base::Time time = timer_.StartWithPrivacy(delay,
      base::BindOnce(&AdsServe::DownloadCatalog, base::Unretained(this)));

  BLOG(1, "Download catalog " << FriendlyDateAndTime(time));
}

uint64_t AdsServe::CatalogLastUpdated() const {
  return catalog_last_updated_;
}

void AdsServe::Reset() {
  timer_.Stop();
  retry_timer_.Stop();

  ResetCatalog();
}

//////////////////////////////////////////////////////////////////////////////

void AdsServe::BuildUrl() {
  switch (_environment) {
    case Environment::PRODUCTION: {
      url_ = PRODUCTION_SERVER;
      break;
    }

    case Environment::STAGING: {
      url_ = STAGING_SERVER;
      break;
    }

    case Environment::DEVELOPMENT: {
      url_ = DEVELOPMENT_SERVER;
      break;
    }
  }

  url_ += CATALOG_PATH;
}

void AdsServe::OnCatalogDownloaded(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  BLOG(7, UrlResponseToString(url, response_status_code, response, headers));

  auto should_retry = false;

  if (response_status_code / 100 == 2) {
    if (!response.empty()) {
      BLOG(1, "Successfully downloaded catalog");
    }

    if (!ProcessCatalog(response)) {
      should_retry = true;
    }
  } else if (response_status_code == 304) {
    BLOG(1, "Catalog is up to date");
  } else {
    BLOG(1, "Failed to download catalog");

    should_retry = true;
  }

  if (should_retry) {
    RetryDownloadingCatalog();
    return;
  }

  retry_timer_.Stop();

  DownloadCatalogAfterDelay();
}

bool AdsServe::ProcessCatalog(const std::string& json) {
  // TODO(Terry Mancey): Refactor function to use callbacks

  BLOG(1, "Parsing catalog");

  Catalog catalog(ads_);
  if (!catalog.FromJson(json)) {
    BLOG(0, "Failed to load catalog");

    BLOG(3, "Failed to parse catalog: " << json);

    return false;
  }

  if (!catalog.HasChanged(bundle_->GetCatalogId())) {
    BLOG(1, "Catalog id " << catalog.GetId() << " matches current catalog id "
        << bundle_->GetCatalogId());

    return true;
  }

  BLOG(1, "Generating bundle");

  if (!bundle_->UpdateFromCatalog(catalog)) {
    BLOG(0, "Failed to generate bundle");

    return false;
  }

  BLOG(1, "Successfully generated bundle");

  auto callback = std::bind(&AdsServe::OnCatalogSaved, this, _1);
  catalog.Save(json, callback);

  auto issuers_info = std::make_unique<IssuersInfo>(catalog.GetIssuers());
  ads_->get_ads_client()->SetCatalogIssuers(std::move(issuers_info));

  return true;
}

void AdsServe::OnCatalogSaved(const Result result) {
  if (result != SUCCESS) {
    // If the catalog fails to save, we will retry the next time we download the
    // catalog

    BLOG(0, "Failed to save catalog");

    return;
  }

  BLOG(3, "Successfully saved catalog");
}

void AdsServe::RetryDownloadingCatalog() {
  const base::Time time = retry_timer_.StartWithBackoff(
      kRetryDownloadingCatalogAfterSeconds,
          base::BindOnce(&AdsServe::DownloadCatalog, base::Unretained(this)));

  BLOG(1, "Retry downloading catalog " << FriendlyDateAndTime(time));
}

void AdsServe::ResetCatalog() {
  BLOG(3, "Resetting catalog");

  Catalog catalog(ads_);
  auto callback = std::bind(&AdsServe::OnCatalogReset, this, _1);
  catalog.Reset(callback);
}

void AdsServe::OnCatalogReset(const Result result) {
  if (result != SUCCESS) {
    BLOG(0, "Failed to reset catalog");

    return;
  }

  BLOG(3, "Successfully reset catalog");
}

}  // namespace ads
