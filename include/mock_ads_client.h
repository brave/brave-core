/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>

#include "../include/ads_client.h"
#include "../include/ads_url_loader.h"
#include "../include/ads_impl.h"

namespace ads {
class Ads;
class CallbackHandler;
}  // namespace ads

namespace bat_ads {

class MockAdsClient : public ads::AdsClient {
 public:
  MockAdsClient();
  ~MockAdsClient() override;

 protected:
  // ads::AdsClient
  void Initialize() override;
  void AppFocused(bool focused) override;
  void TabUpdate() override;
  void RecordUnIdle() override;
  void RemoveAllHistory() override;
  void SaveCachedInfo() override;
  void ConfirmAdUUIDIfAdEnabled(bool enabled) override;
  void TestShoppingData(const std::string& url) override;
  void TestSearchState(const std::string& url) override;
  void RecordMediaPlaying(bool active, uint64_t tabId) override;
  void ClassifyPage(uint64_t windowId) override;
  void ChangeLocale(const std::string& locale) override;
  void CollectActivity() override;
  void RetrieveSSID(uint64_t error, const std::string& ssid) override;
  void InitializeCatalog() override;
  void CheckReadyAdServe(uint64_t windowId, bool forceP) override;
  void ServeSampleAd(uint64_t windowId) override;

  std::unique_ptr<ads::Ads> ads_;
};

}  // namespace bat_ads
