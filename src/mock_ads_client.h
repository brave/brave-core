/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_MOCK_ADS_CLIENT_H_
#define BAT_ADS_MOCK_ADS_CLIENT_H_

#include <sstream>
#include <string>
#include <memory>

#include "bat/ads/ads_client.h"
#include "bat/ads/ads.h"
#include "bat/ads/client_info.h"
#include "bat/ads/bundle_state.h"
#include "bat/ads/url_components.h"
#include "catalog_state.h"

namespace ads {

class Ads;

class MockAdsClient : public AdsClient {
 public:
  MockAdsClient();
  ~MockAdsClient() override;

  std::unique_ptr<Ads> ads_;

 protected:
  // AdsClient
  bool IsAdsEnabled() const override;
  const std::string GetAdsLocale() const override;
  uint64_t GetAdsPerHour() const override;
  uint64_t GetAdsPerDay() const override;

  void SetIdleThreshold(const int threshold) override;

  bool IsNetworkConnectionAvailable() override;

  void GetClientInfo(ClientInfo *client_info) const override;

  const std::vector<std::string> GetLocales() const override;

  void LoadUserModelForLocale(
      const std::string& locale,
      OnLoadCallback callback) const override;

  const std::string GenerateUUID() const override;

  bool IsForeground() const override;

  bool IsNotificationsAvailable() const override;
  void ShowNotification(std::unique_ptr<NotificationInfo> info) override;

  void SetCatalogIssuers(std::unique_ptr<IssuersInfo> info) override;
  bool IsConfirmationsReadyToShowAds() override;
  void AdSustained(std::unique_ptr<NotificationInfo> info) override;

  uint32_t SetTimer(const uint64_t time_offset) override;
  void KillTimer(const uint32_t timer_id) override;

  void URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const URLRequestMethod method,
      URLRequestCallback callback) override;

  void Save(
      const std::string& name,
      const std::string& value,
      OnSaveCallback callback) override;
  void SaveBundleState(
      std::unique_ptr<BundleState> state,
      OnSaveCallback callback) override;

  void Load(const std::string& name, OnLoadCallback callback) override;

  const std::string LoadJsonSchema(const std::string& name) override;

  void Reset(const std::string& name, OnResetCallback callback) override;

  void GetAds(
      const std::string& region,
      const std::string& category,
      OnGetAdsCallback callback) override;

  void LoadSampleBundle(OnLoadSampleBundleCallback callback) override;

  bool GetUrlComponents(
      const std::string& url,
      UrlComponents* components) const override;

  void EventLog(const std::string& json) override;

  std::unique_ptr<LogStream> Log(
      const char* file,
      const int line,
      const LogLevel log_level) const override;

 private:
  void LoadBundleState();
  void OnBundleStateLoaded(const Result result, const std::string& json);
  std::unique_ptr<BundleState> bundle_state_;

  void LoadSampleBundleState();
  void OnSampleBundleStateLoaded(const Result result, const std::string& json);
  std::unique_ptr<BundleState> sample_bundle_state_;

  bool WriteValueToDisk(
    const std::string& path,
    const std::string& value) const;
};

}  // namespace ads

#endif  // BAT_ADS_MOCK_ADS_CLIENT_H_
