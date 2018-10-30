/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>
#include <memory>

#include "ads_impl.h"
#include "ads_client.h"

namespace state {
class Bundle;
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
      std::shared_ptr<state::Bundle> bundle);

  ~AdsServe();

  void DownloadCatalog();

  void ResetNextCatalogCheck();

 private:
  std::string url_;
  void BuildUrl();

  uint64_t next_catalog_check_;
  void UpdateNextCatalogCheck();

  void OnCatalogDownloaded(
      const std::string& url,
      const int response_status_code,
      const std::string& response,
      const std::map<std::string, std::string>& headers);

  void OnCatalogSaved(const ads::Result result);

  rewards_ads::AdsImpl* ads_;  // NOT OWNED
  ads::AdsClient* ads_client_;  // NOT OWNED

  std::shared_ptr<state::Bundle> bundle_;
};

}  // namespace catalog
