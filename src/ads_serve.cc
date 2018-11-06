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
  // TODO(bridiver) - why are we calling to get this every time? I'm not sure
  // that we should even be passing this info in the first place
  ClientInfo client_info = ads_client_->GetClientInfo();

  url_ = _is_production ? ADS_PRODUCTION_SERVER : ADS_STAGING_SERVER;

  url_ += "?braveVersion=" + client_info.application_version;
  url_ += "&platform=" + client_info.platform;
  url_ += "&platformVersion=" + client_info.platform_version;
}

void AdsServe::DownloadCatalog() {
  auto url_session = ads_client_->URLSessionTask(
    url_,
    {},
    "",
    "",
    URLSession::Method::GET,
    std::bind(
      &AdsServe::OnCatalogDownloaded,
      this,
      _1,
      _2,
      _3,
      _4));
}

void AdsServe::OnCatalogDownloaded(
    const std::string& url,
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  if (response_status_code / 100 == 2) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Catalog downloaded', [ 'version', 'catalog', 'status' ]

    // TODO(Brian Johnson): I have added the catalog class back to the project
    // can you please change the code below to instantiate a catalog which
    // should be responsible for the catalog state as there is custom logic
    // in the catalog class concerning catalog_id's and I am sure further logic
    // will be added in the future, the catalog class does have a shared_ptr
    // which will also need changing
    CATALOG_STATE catalog_state;
    if (!catalog_state.LoadFromJson(response)) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Failed to parse catalog'

      RetryDownloadingCatalog();
      return;
    }

    // TODO(Terry Mancey): Implement Log (#44)
    // 'Catalog parsed', underscore.extend(underscore.clone(header),
    // { status: 'processed', campaigns: underscore.keys(campaigns).length,
    // creativeSets: underscore.keys(creativeSets).length

    auto current_catalog_version = bundle_->GetCatalogVersion();
    if (current_catalog_version != 0 &&
        current_catalog_version <= catalog_state.version) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Current catalog is the same or a newer version'

      RetryDownloadingCatalog();
      return;
    }

    if (!bundle_->GenerateFromCatalog(catalog_state)) {
      // TODO(Terry Mancey): Implement Log (#44)
      // 'Failed to generate bundle'

      RetryDownloadingCatalog();
      return;
    }

    // TODO(Terry Mancey): Implement Log (#44)
    // 'Generated bundle'

    ads_client_->Save("catalog.json", response,
      std::bind(&AdsServe::OnCatalogSaved, this, _1));

    ads_->SaveCachedInfo();

    UpdateNextCatalogCheck();
  } else if (response_status_code == 304) {
    // TODO(Terry Mancey): Implement Log (#44)
    // 'Catalog current', { method, server, path }

    UpdateNextCatalogCheck();
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

    LOG(ads_client_, LogLevel::WARNING) <<
      "Failed to download catalog from " << url <<
      " (" << response_status_code << "): " <<
      response << " " << formatted_headers;

    RetryDownloadingCatalog();
  }
}

void AdsServe::RetryDownloadingCatalog() {
  ads_->StartCollectingActivity(kOneHourInSeconds);
}

void AdsServe::Reset() {
  next_catalog_check_ = 0;

  // TODO(Brian Johnson): See the other TODO I created as this function should
  // only reset the next_catalog_check_ and should be renamed back to its
  // original name
  ads_client_->Reset("catalog.json",
    std::bind(&AdsServe::OnCatalogReset, this, _1));
}

void AdsServe::UpdateNextCatalogCheck() {
  next_catalog_check_ = bundle_->GetCatalogPing();
  ads_->StartCollectingActivity(next_catalog_check_);
}

//////////////////////////////////////////////////////////////////////////////

void AdsServe::OnCatalogSaved(const Result result) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::WARNING) << "Failed to save catalog";
  }
}

void AdsServe::OnCatalogReset(const Result result) {
  if (result == Result::FAILED) {
    LOG(ads_client_, LogLevel::WARNING) << "Failed to reset catalog";
  }
}

}  // namespace ads
