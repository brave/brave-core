/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CATALOG_CATALOG_H_
#define BAT_ADS_INTERNAL_CATALOG_CATALOG_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/catalog/catalog_campaign_info.h"

namespace ads {

class AdsImpl;
struct CatalogState;
struct CatalogIssuersInfo;

class Catalog {
 public:
  explicit Catalog(
      AdsImpl* ads);

  ~Catalog();

  bool FromJson(
      const std::string& json);

  bool HasChanged(
      const std::string& current_catalog_id);

  std::string GetId() const;
  uint64_t GetVersion() const;
  uint64_t GetPing() const;
  CatalogCampaignList GetCampaigns() const;
  CatalogIssuersInfo GetIssuers() const;

  void Save(
      const std::string& json,
      ResultCallback callback);

  const std::string& get_last_message() const;

 private:
  AdsImpl* ads_;  // NOT OWNED

  std::unique_ptr<CatalogState> catalog_state_;

  std::string last_message_;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CATALOG_CATALOG_H_
