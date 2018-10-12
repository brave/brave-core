/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/catalog_ads_serve.h"
#include "../include/ads_impl.h"
#include "../include/ads.h"
#include "../include/static_values.h"

namespace catalog {

AdsServe::AdsServe(
    rewards_ads::AdsImpl* ads,
    ads::AdsClient* ads_client,
    std::shared_ptr<state::Catalog> catalog) :
      ads_(ads),
      ads_client_(ads_client),
      catalog_(catalog) {
  BuildURL();
}

AdsServe::~AdsServe() = default;

void AdsServe::BuildURL() {
  ads::ClientInfo client_info;
  ads_client_->GetClientInfo(client_info);

  url_ = ads::is_production ? ADS_PRODUCTION_SERVER : ADS_STAGING_SERVER;

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
    ads::URLSession::Method::GET,
    std::bind(
      &AdsServe::OnCatalogDownloaded,
      this,
      std::placeholders::_1,
      std::placeholders::_2,
      std::placeholders::_3));
}

void AdsServe::OnCatalogDownloaded(
    const int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  bool success = false;

  if (response_status_code / 100 == 2) {  // successful
    success = catalog_->LoadState(response);
  } else if (response_status_code == 304) {  // not modified
    // TODO(Terry Mancey): Implement UserModelLog (#44)
  } else {  // failed
    // TODO(Terry Mancey): Implement UserModelLog (#44)
  }

  if (success) {
    UpdateNextCatalogCheck();
  }

  ads::Result result = success ? ads::Result::ADS_OK : ads::Result::ADS_ERROR;
  ads_->OnCatalogStateLoaded(result, response);
}

void AdsServe::ResetNextCatalogCheck() {
  next_catalog_check_ = 0;
}

void AdsServe::UpdateNextCatalogCheck() {
  next_catalog_check_ = catalog_->GetPing();
}

uint64_t AdsServe::NextCatalogCheck() const {
  return next_catalog_check_;
}

}  // namespace catalog
