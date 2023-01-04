/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_INFO_H_

#include <string>

#include "base/time/time.h"
#include "bat/ads/internal/catalog/campaign/catalog_campaign_info.h"

namespace ads {

struct CatalogInfo final {
  CatalogInfo();

  CatalogInfo(const CatalogInfo& other);
  CatalogInfo& operator=(const CatalogInfo& other);

  CatalogInfo(CatalogInfo&& other) noexcept;
  CatalogInfo& operator=(CatalogInfo&& other) noexcept;

  ~CatalogInfo();

  bool operator==(const CatalogInfo& other) const;
  bool operator!=(const CatalogInfo& other) const;

  std::string id;
  int version = 0;
  base::TimeDelta ping;
  CatalogCampaignList campaigns;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_INFO_H_
