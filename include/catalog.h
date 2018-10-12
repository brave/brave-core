/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>

#include "../include/ads_impl.h"
#include "../include/catalog_state.h"

namespace rewards_ads {
class AdsImpl;
}  // namespace rewards_ads

namespace state {

class Catalog: public ads::CallbackHandler {
 public:
  Catalog(rewards_ads::AdsImpl* ads, ads::AdsClient* ads_client);
  ~Catalog();

  bool LoadState(const std::string &json);
  void SaveState();

  std::string GetCatalogId() const;

  int64_t GetVersion() const;

  int64_t GetPing() const;

  void Reset();

 private:
  void OnCatalogStateSaved(const ads::Result result) override;

  rewards_ads::AdsImpl* ads_;  // NOT OWNED
  ads::AdsClient* ads_client_;  // NOT OWNED

  std::shared_ptr<CATALOG_STATE> catalog_state_;
};

}  // namespace state
