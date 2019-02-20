/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "ads_serve.h"
#include "static_values.h"
#include "bundle.h"
#include "logging.h"
#include "math_helper.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ads {

AdsServe::AdsServe(AdsImpl* ads, AdsClient* ads_client, Bundle* bundle) :
    url_(""),
    next_catalog_check_(0),
    next_retry_start_timer_in_(0),
    catalog_last_updated_(0),
    ads_(ads),
    ads_client_(ads_client),
    bundle_(bundle) {
  BuildUrl();
}

AdsServe::~AdsServe() = default;

void AdsServe::BuildUrl() {
  if (_is_production) {
    url_ = PRODUCTION_SERVER;
  } else {
    url_ = STAGING_SERVER;
  }

  url_ += CATALOG_PATH;
}

void AdsServe::DownloadCatalog() {
  auto callback = std::bind(&AdsServe::OnCatalogDownloaded,
      this, url_, _1, _2, _3);

  ads_client_->URLRequest(url_, {}, "", "", URLRequestMethod::GET, callback);
}

void AdsServe::OnCatalogDownloaded(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  auto should_retry = false;

  if (response_status_code / 100 == 2) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Catalog downloaded', [ 'version', 'catalog', 'status' ]

    if (!response.empty()) {
      LOG(INFO) << "Successfully downloaded catalog";
    }

    if (!ProcessCatalog(response)) {
      should_retry = true;
    }
  } else if (response_status_code == 304) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Catalog current', { method, server, path }

    LOG(INFO) << "Catalog is already up to dates";
  } else {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Catalog download failed', { error, method, server, path }

    std::string formatted_headers = "";
    for (auto header = headers.begin(); header != headers.end(); ++header) {
      formatted_headers += header->first + ": " + header->second;
      if (header != headers.end()) {
        formatted_headers += ", ";
      }
    }

    LOG(ERROR) << "Failed to download catalog from:"
        << std::endl << "  url: " << url
        << std::endl << "  response_status_code: " << response_status_code
        << std::endl << "  response: " << response
        << std::endl << "  headers: " << formatted_headers;

    should_retry = true;
  }

  if (should_retry) {
    RetryDownloadingCatalog();
    return;
  }
}

uint64_t AdsServe::CatalogLastUpdated() const {
  return catalog_last_updated_;
}

void AdsServe::Reset() {
  ads_->StopCollectingActivity();

  next_retry_start_timer_in_ = 0;

  next_catalog_check_ = 0;

  ResetCatalog();
}

void AdsServe::UpdateNextCatalogCheck() {
  next_retry_start_timer_in_ = 0;

  auto ping = bundle_->GetCatalogPing();

  // Add randomized delay so that the Ad server can't correlate users by timing
  auto rand_delay = helper::Math::Random(ping / 10);
  next_catalog_check_ = ping + rand_delay;

  ads_->StartCollectingActivity(next_catalog_check_);
}

//////////////////////////////////////////////////////////////////////////////

bool AdsServe::ProcessCatalog(const std::string& json) {
  // TODO(Terry Mancey): Refactor function to use callbacks

  Catalog catalog(ads_client_);

  LOG(INFO) << "Parsing catalog";

  if (!catalog.FromJson(json)) {
    return false;
  }

  LOG(INFO) << "Catalog parsed";

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Catalog parsed', underscore.extend(underscore.clone(header),
  // { status: 'processed', campaigns: underscore.keys(campaigns).length,
  // creativeSets: underscore.keys(creativeSets).length

  if (!catalog.HasChanged(bundle_->GetCatalogId())) {
    LOG(WARNING) << "Catalog id " << catalog.GetId() <<
        " matches current catalog id " << bundle_->GetCatalogId();

    return true;
  }

  LOG(INFO) << "Generating bundle";

  if (!bundle_->UpdateFromCatalog(catalog)) {
    LOG(ERROR) << "Failed to generate bundle";

    return false;
  }

  auto callback = std::bind(&AdsServe::OnCatalogSaved, this, _1);
  catalog.Save(json, callback);

  auto issuers_info = std::make_unique<IssuersInfo>(catalog.GetIssuers());
  ads_client_->SetCatalogIssuers(std::move(issuers_info));

  return true;
}

void AdsServe::OnCatalogSaved(const Result result) {
  if (result != SUCCESS) {
    // If the catalog fails to save, we will retry the next time we collect
    // activity

    LOG(ERROR) << "Failed to save catalog";

    return;
  }

  LOG(INFO) << "Successfully saved catalog";
}

void AdsServe::RetryDownloadingCatalog() {
  LOG(INFO) << "Retry downloading catalog";

  if (next_retry_start_timer_in_ == 0) {
    if (ads_->IsMobile()) {
      next_retry_start_timer_in_ = 2 * kOneMinuteInSeconds;
    } else {
      next_retry_start_timer_in_ = kOneMinuteInSeconds;
    }
  } else {
    next_retry_start_timer_in_ *= 2;
  }

  ads_->StartCollectingActivity(next_retry_start_timer_in_);
}

void AdsServe::ResetCatalog() {
  LOG(INFO) << "Resetting catalog to default state";

  Catalog catalog(ads_client_);
  auto callback = std::bind(&AdsServe::OnCatalogReset, this, _1);
  catalog.Reset(callback);
}

void AdsServe::OnCatalogReset(const Result result) {
  if (result != SUCCESS) {
    LOG(ERROR) << "Failed to reset catalog";

    return;
  }

  LOG(INFO) << "Successfully reset catalog";
}

}  // namespace ads
