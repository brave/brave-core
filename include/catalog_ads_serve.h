/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <ctime>
#include <map>

#include "../include/catalog.h"

namespace state {
class Catalog;
}  // namespace state

namespace rewards_ads {
class AdsImpl;
}  // namespace rewards_ads

namespace catalog {

class AdsServe: public ads::CallbackHandler {
 public:
  AdsServe(
      rewards_ads::AdsImpl* ads,
      ads::AdsClient* ads_client,
      std::shared_ptr<state::Catalog> catalog);

  ~AdsServe();

  void BuildURL();

  void DownloadCatalog();

  void OnCatalogDownloaded(
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void ResetNextCatalogCheck();

  void UpdateNextCatalogCheck();
  uint64_t NextCatalogCheck() const;

 private:

  std::string url_;

  uint64_t next_catalog_check_;

  rewards_ads::AdsImpl* ads_;  // NOT OWNED
  ads::AdsClient* ads_client_;  // NOT OWNED

  std::shared_ptr<state::Catalog> catalog_;
};

}  // namespace catalog
