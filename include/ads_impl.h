/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <map>

#include "../include/ads.h"
#include "../include/callback_handler.h"
#include "../include/ads_client.h"

namespace ads_bat_client {
class BatClient;
}

namespace bat_ads {

class AdsImpl : public ads::Ads, public ads::CallbackHandler {
 public:
  explicit AdsImpl(ads::AdsClient* ads_client);
  ~AdsImpl() override;

  // Not copyable, not assignable
  AdsImpl(const AdsImpl&) = delete;
  AdsImpl& operator=(const AdsImpl&) = delete;

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
  void InitializeCatalog() override;
  void RetrieveSSID(uint64_t error, const std::string& ssid) override;
  void CheckReadyAdServe(uint64_t windowId, bool forceP) override;
  void ServeSampleAd(uint64_t windowId) override;

  void OnTimer(uint32_t timer_id) override;

  void SaveState(const std::string& json);

  void SetCampaignInfo(std::unique_ptr<catalog::CampaignInfo> info,
      ads::CampaignInfoCallback callback) override;
  void OnSetCampaignInfo(ads::CampaignInfoCallback callback,
      ads::Result result, std::unique_ptr<catalog::CampaignInfo> info);
  void GetCampaignInfo(const catalog::CampaignInfoFilter& filter,
      ads::CampaignInfoCallback callback) override;

  void SetCreativeSetInfo(std::unique_ptr<catalog::CreativeSetInfo> info,
      ads::CreativeSetInfoCallback callback) override;
  void OnSetCreativeSetInfo(ads::CreativeSetInfoCallback callback,
      ads::Result result, std::unique_ptr<catalog::CreativeSetInfo> info);
  void GetCreativeSetInfo(const catalog::CreativeSetInfoFilter& filter,
      ads::CreativeSetInfoCallback callback) override;

  std::string URIEncode(const std::string& value) override;

  std::unique_ptr<ads::AdsURLLoader> LoadURL(const std::string& url,
      const std::vector<std::string>& headers, const std::string& content,
      const std::string& contentType, const ads::URL_METHOD& method,
      ads::CallbackHandler* handler);

 private:
  bool focused_;

  std::string bundle_path_;
  std::string catalog_path_;

  ads::AdsClient* ads_client_;
  std::unique_ptr<ads_bat_client::BatClient> bat_client_;
};

}  // namespace bat_ads
