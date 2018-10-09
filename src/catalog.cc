/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/catalog.h"
#include "../include/ads.h"

catalog::Catalog::Catalog(bat_ads::AdsImpl* ads) :
    ads_(ads) {
}

catalog::Catalog::~Catalog() = default;

std::string catalog::Catalog::GetCatalogId() {
  // TODO(Terry Mancey): Get catalog id
  return "";
}

int64_t catalog::Catalog::GetVersion() {
  // TODO(Terry Mancey): Get version
  return 0;
}

bool catalog::Catalog::Parse(const std::string& json) {
  // TODO(Terry Mancey): Parse JSON and save to database (#4)
  // TODO(Terry Mancey): Parse JSON and merge with database (#5)

  return false;
}
