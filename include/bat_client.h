  /* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "../include/ads_impl.h"
#include "../include/catalog.h"
#include "../include/state_user_model.h"

namespace bat_ads {
class AdsImpl;
}  // namespace bat_ads

namespace ads_bat_client {

class BatClient {
 public:
  explicit BatClient(bat_ads::AdsImpl* ads);
  ~BatClient();

  bool LoadState(const std::string& json);

  // Called when the catalog server has returned a result.
  // If the result is good, an upcall is made to save the catalog state
  // and save the userModel state
  void ApplyCatalog(const catalog::Catalog& catalog, bool bootP);

 private:
  void SaveState();

  bat_ads::AdsImpl* ads_;  // NOT OWNED

  std::unique_ptr<USER_MODEL_STATE_ST> userModelState_;
};

}  // namespace ads_bat_client
