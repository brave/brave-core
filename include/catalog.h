/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "../include/catalog_campaign.h"
#include "../include/ads_impl.h"
#include "../include/callback_handler.h"
#include "../include/catalog_campaign.h"

namespace catalog {

class Catalog {
 public:
  explicit Catalog(bat_ads::AdsImpl* ads);
  ~Catalog();

  std::string GetCatalogId();

  int64_t GetVersion();

  bool Parse(const std::string& json);

 private:
  bat_ads::AdsImpl* ads_;  // NOT OWNED
};

}  // namespace catalog
