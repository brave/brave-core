/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CATALOG_CATALOG_H_
#define BAT_ADS_INTERNAL_CATALOG_CATALOG_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "bat/ads/internal/catalog/catalog_campaign_info.h"

namespace ads {

struct CatalogState;
struct CatalogIssuersInfo;

class Catalog {
 public:
  Catalog();

  ~Catalog();

  bool FromJson(
      const std::string& json);

  bool HasChanged(
      const std::string& catalog_id) const;

  std::string GetId() const;
  int GetVersion() const;
  int64_t GetPing() const;
  CatalogIssuersInfo GetIssuers() const;
  CatalogCampaignList GetCampaigns() const;

 private:
  std::unique_ptr<CatalogState> catalog_state_;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CATALOG_CATALOG_H_
