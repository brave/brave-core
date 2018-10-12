/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "static_values.h"
#include "../include/catalog_ads_serve.h"
#include "../include/ads.h"

catalog::AdsServe::AdsServe(const std::string& path) : ping_(0) {
  if (ads::is_production) {
    url_ = ADS_PRODUCTION_SERVER;
  } else {
    url_ = ADS_STAGING_SERVER;
  }
}

catalog::AdsServe::~AdsServe() = default;

void catalog::AdsServe::set_ping(std::time_t ping) {
  ping_ = ping;
}

std::time_t catalog::AdsServe::NextCatalogCheck() {
  auto now = std::time(nullptr);
  return now + ping_;
}

bool catalog::AdsServe::DownloadCatalog() {
  return false;
}
