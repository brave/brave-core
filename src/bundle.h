/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "bat/ads/ads_client.h"
#include "bat/ads/callback_handler.h"
#include "bat/ads/bundle_state.h"
#include "catalog_state.h"

namespace ads {

class Bundle: public ads::CallbackHandler {
 public:
  explicit Bundle(ads::AdsClient* ads_client);
  ~Bundle();

  bool LoadJson(const std::string& json);  // Deserialize
  void SaveJson();  // Serialize
  void Save();

  bool GenerateFromCatalog(const std::shared_ptr<CATALOG_STATE> catalog_state);

  void Reset();

  std::string GetCatalogId() const;
  uint64_t GetCatalogVersion() const;
  uint64_t GetCatalogPing() const;

 private:
  void OnBundleSaved(const ads::Result result);

  ads::AdsClient* ads_client_;  // NOT OWNED

  std::shared_ptr<BUNDLE_STATE> bundle_state_;
};

}  // namespace ads
