/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_IMPL_
#define BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_IMPL_

#include <memory>
#include <string>
#include <vector>

#include "bat/ads/ads_client.h"
#include "brave/components/brave_ads/browser/ads_service.h"

class Profile;

namespace ads {
class Ads;
}

namespace brave_ads {

class AdsServiceImpl : public AdsService, ads::AdsClient {
 public:
  explicit AdsServiceImpl(Profile* profile);
  ~AdsServiceImpl() override;

  bool is_enabled() override;

 private:
  void GetClientInfo(ads::ClientInfo& client_info) const override {}
  void LoadUserModel(ads::CallbackHandler* callback_handler) override {}
  std::string SetLocale(const std::string& locale) override;
  void GetLocales(std::vector<std::string>& locales) const override {}
  void GenerateAdUUID(std::string& ad_uuid) const override {}
  void GetSSID(std::string& ssid) const override {}
  void ShowAd(const std::unique_ptr<ads::AdInfo> info) override {}
  void SetTimer(const uint64_t time_offset, uint32_t& timer_id) override {}
  void KillTimer(uint32_t& timer_id) override {};
  std::unique_ptr<ads::URLSession> URLSessionTask(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const ads::URLSession::Method& method,
      ads::URLSessionCallbackHandlerCallback callback) override;
  void LoadSettings(ads::CallbackHandler* callback_handler) override {};
  void SaveClient(
      const std::string& json,
      ads::CallbackHandler* callback_handler) override {}
  void LoadClient(ads::CallbackHandler* callback_handler) override {}
  void SaveCatalog(
      const std::string& json,
      ads::CallbackHandler* callback_handler) override {}
  void LoadCatalog(ads::CallbackHandler* callback_handler) override {};
  void ResetCatalog() override {};
  void SaveBundle(
      const ads::BUNDLE_STATE& bundle_state,
      ads::CallbackHandler* callback_handler) override {}
  void SaveBundle(
      const std::string& json,
      ads::CallbackHandler* callback_handler) override {}
  void LoadBundle(ads::CallbackHandler* callback_handler) override {}
  void GetAds(
      const std::string& winning_category,
      ads::CallbackHandler* callback_handler) override {}
  void GetSampleCategory(ads::CallbackHandler* callback_handler) override {}
  void GetUrlComponents(
      const std::string& url,
      ads::UrlComponents& components) const override {}
  void EventLog(const std::string& json) override {}
  void DebugLog(const ads::LogLevel log_level, const char *fmt, ...) const override {}

  Profile* profile_;  // NOT OWNED
  std::unique_ptr<ads::Ads> ads_;

  DISALLOW_COPY_AND_ASSIGN(AdsServiceImpl);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_ADS_SERVICE_IMPL_
