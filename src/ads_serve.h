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

class AdsImpl;
class Bundle;

class AdsServe {
 public:
  AdsServe(
      AdsImpl* ads,
      AdsClient* ads_client,
      Bundle* bundle);

  ~AdsServe();

  void DownloadCatalog();

  void Reset();

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
  void ProcessCatalog(const std::string& json);
  void RetryDownloadingCatalog();

  void OnCatalogSaved(const Result result);
  void OnCatalogReset(const Result result);

  AdsImpl* ads_;  // NOT OWNED
  AdsClient* ads_client_;  // NOT OWNED
  Bundle* bundle_;  // NOT OWNED
};

}  // namespace ads
