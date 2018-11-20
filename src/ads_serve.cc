/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ads_serve.h"
#include "static_values.h"
#include "bundle.h"
#include "logging.h"

using namespace std::placeholders;

namespace ads {

AdsServe::AdsServe(
    AdsImpl* ads,
    AdsClient* ads_client,
    Bundle* bundle) :
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

  url_ += "&platform=";
  url_ += client_info.platform;

  url_ += "&platformVersion=";
  url_ += client_info.platform_version;
}

void AdsServe::DownloadCatalog() {
  ads_client_->URLRequest(
      url_,
      {},
      "",
      "",
      URLRequestMethod::GET,
      std::bind(
          &AdsServe::OnCatalogDownloaded,
          this,
          url_,
          _1,
          _2,
          _3));
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
    LOG(LogLevel::INFO) << "Successfully downloaded catalog";

    if (!ProcessCatalog(response)) {
      should_retry = true;
    }
  } else if (response_status_code == 304) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Catalog current', { method, server, path }

    LOG(LogLevel::INFO) << "Catalog is already up to date";
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

    LOG(LogLevel::ERROR) << "Failed to download catalog from "
      << url << " (" << response_status_code << "): " << response << " " <<
      formatted_headers;

    should_retry = true;
  }

  if (should_retry) {
    RetryDownloadingCatalog();
    return;
  }

  UpdateNextCatalogCheck();
}

void AdsServe::Reset() {
  ads_->StopCollectingActivity();

  next_catalog_check_ = 0;

  ResetCatalog();
}

void AdsServe::UpdateNextCatalogCheck() {
  next_catalog_check_ = bundle_->GetCatalogPing();
  ads_->StartCollectingActivity(next_catalog_check_);
}

//////////////////////////////////////////////////////////////////////////////

bool AdsServe::ProcessCatalog(const std::string& json) {
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
  if (_is_debug) {
    ads_->StartCollectingActivity(kDebugOneHourInSeconds);
  } else {
    ads_->StartCollectingActivity(kOneHourInSeconds);
  }
}

void AdsServe::ResetCatalog() {
  Catalog catalog(ads_client_, bundle_);
  catalog.Reset();
}

}  // namespace ads
