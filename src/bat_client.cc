/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/bat_client.h"
#include "../include/ads_impl.h"

namespace ads_bat_client {

BatClient::BatClient(bat_ads::AdsImpl* ads) : ads_(ads) {
}

BatClient::~BatClient() = default;

void BatClient::SetAdsEnabled(bool enabled) {
  // TODO(Terry Mancey): Implement SetAdsEnabled
}

bool BatClient::IsAdsEnabled() const {
  // TODO(Terry Mancey): Implement IsAdsEnabled

  return false;
}

void BatClient::ApplyCatalog(const catalog::Catalog& catalog, bool bootP) {
  // TODO(Terry Mancey): Implement ApplyCatalog
}

}  // namespace ads_bat_client
