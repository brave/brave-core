/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/catalog_frequency_cap.h"

#include "bat/ads/internal/ad_server/ad_server.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/bundle/bundle.h"
#include "bat/ads/internal/catalog/catalog_issuers_info.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

CatalogFrequencyCap::CatalogFrequencyCap(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

CatalogFrequencyCap::~CatalogFrequencyCap() = default;

bool CatalogFrequencyCap::ShouldAllow() {
  if (!DoesRespectCap()) {
    return false;
  }

  return true;
}

std::string CatalogFrequencyCap::get_last_message() const {
  return last_message_;
}

bool CatalogFrequencyCap::DoesRespectCap() {
  const CatalogIssuersInfo catalog_issuers =
      ads_->get_confirmations()->GetCatalogIssuers();
  if (!catalog_issuers.IsValid()) {
    last_message_ = "Catalog issuers not initialized";
    return false;
  }

  if (!ads_->get_bundle()->Exists()) {
    last_message_ = "Bundle does not exist";
    return false;
  }

  if (ads_->get_bundle()->IsOlderThanOneDay()) {
    last_message_ = "Bundle is out of date";
    ads_->get_ad_server()->MaybeFetch();
    return false;
  }

  return true;
}

}  // namespace ads
