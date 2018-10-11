/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../include/bat_client.h"
#include "../include/ads_impl.h"

namespace ads_bat_client {

BatClient::BatClient(bat_ads::AdsImpl* ads) :
    ads_(ads),
    userModelState_(new USER_MODEL_STATE_ST()) {
}

BatClient::~BatClient() = default;

bool BatClient::LoadState(const std::string& json) {
  USER_MODEL_STATE_ST state;
  if (!LoadFromJson(state, json.c_str())) {
    return false;
  }

  userModelState_.reset(new USER_MODEL_STATE_ST(state));

  return true;
}

void BatClient::ApplyCatalog(const catalog::Catalog& catalog, bool bootP) {
  // TODO(Terry Mancey): Implement ApplyCatalog
}

void BatClient::SaveState() {
  std::string json;
  SaveToJsonString(*userModelState_, json);
  ads_->SaveState(json);
}

}  // namespace ads_bat_client
