/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ads_serve.h"
#include "static_values.h"
#include "bundle.h"
#include "logging.h"
#include "math_helper.h"

using namespace std::placeholders;

namespace ads {

AdsServe::AdsServe(
    AdsImpl* ads,
    AdsClient* ads_client,
    Bundle* bundle) :
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
  ClientInfo client_info;
  ads_client_->GetClientInfo(&client_info);

  url_ = _is_production ? PRODUCTION_SERVER : STAGING_SERVER;
  url_ += CATALOG_PATH;

  url_ += "?braveVersion=";
  url_ += client_info.application_version;

  auto platform = client_info.GetPlatformName();
  if (!platform.empty()) {
    url_ += "&platform=";
    url_ += platform;
  }

  url_ += "&platformVersion=";
  url_ += client_info.platform_version;
}

void AdsServe::DownloadCatalog() {
  auto callback = std::bind(&AdsServe::OnCatalogDownloaded, this,
    url_, _1, _2, _3);

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
    LOG(INFO) << "Successfully downloaded catalog";

    if (!ProcessCatalog(response)) {
      should_retry = true;
    }
  } else if (response_status_code == 304) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Catalog current', { method, server, path }

    LOG(INFO) << "Catalog is already up to date";
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

    LOG(ERROR) << "Failed to download catalog from " << url << " (" <<
      response_status_code << "): " << response << " " << formatted_headers;

    should_retry = true;
  }

  if (should_retry) {
    RetryDownloadingCatalog();
    return;
  }

  UpdateNextCatalogCheck();
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
  auto rand_delay = helper::Math::Random(ping / 10);

  // Add randomized delay so that the Ad server can't correlate users by timing
  next_catalog_check_ = ping + rand_delay;

  ads_->StartCollectingActivity(next_catalog_check_);
}

//////////////////////////////////////////////////////////////////////////////

bool AdsServe::ProcessCatalog(const std::string& json) {
  // TODO(Terry Mancey): Refactor function to use callbacks

  Catalog catalog(ads_client_, bundle_);

  if (!catalog.FromJson(json)) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Failed to parse catalog'

    return false;
  }

  // TODO(Terry Mancey): Implement Log (#44)
  // 'Catalog parsed', underscore.extend(underscore.clone(header),
  // { status: 'processed', campaigns: underscore.keys(campaigns).length,
  // creativeSets: underscore.keys(creativeSets).length

  if (!bundle_->UpdateFromCatalog(catalog)) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Failed to generate bundle'

    return false;
  }

  catalog.Save(json);

  return true;
}

void AdsServe::RetryDownloadingCatalog() {
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
  Catalog catalog(ads_client_, bundle_);
  catalog.Reset();
}

}  // namespace ads
