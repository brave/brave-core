/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>
#include <memory>

#include "ads_impl.h"
#include "bat/ads/ads_client.h"

namespace ads {
class Bundle;
}  // namespace ads

namespace ads {
class AdsImpl;
}  // namespace ads

namespace ads {

class AdsServe: public ads::CallbackHandler {
 public:
  AdsServe(
      ads::AdsImpl* ads,
      ads::AdsClient* ads_client,
      std::shared_ptr<Bundle> bundle);

  ~AdsServe();

  void DownloadCatalog();
  void RetryDownloadingCatalog();

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

  ads::AdsImpl* ads_;  // NOT OWNED
  ads::AdsClient* ads_client_;  // NOT OWNED

  std::shared_ptr<Bundle> bundle_;
};

}  // namespace ads
