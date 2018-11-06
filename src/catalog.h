/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "bat/ads/ads_client.h"
#include "catalog_state.h"
#include "bat/ads/callback_handler.h"

namespace ads {

class AdsImpl;

class Catalog: public CallbackHandler {
 public:
  explicit Catalog(AdsClient* ads_client);
  ~Catalog();

  bool LoadJson(const std::string& json);  // Deserialize

  const std::shared_ptr<CATALOG_STATE> GetCatalogState() const;

 private:
  AdsClient* ads_client_;  // NOT OWNED

  std::shared_ptr<CATALOG_STATE> catalog_state_;
};

}  // namespace ads
