/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "bat/ads/ads_client.h"

@protocol NativeAdsClientBridge;

class NativeAdsClient : public ads::AdsClient {
public:
  NativeAdsClient(id<NativeAdsClientBridge> bridge);
  ~NativeAdsClient() override;

private:
  __unsafe_unretained id<NativeAdsClientBridge> bridge_;

  void ConfirmAd(std::unique_ptr<ads::NotificationInfo> info) override;
  void EventLog(const std::string & json) override;
  const std::string GenerateUUID() const override;
  void GetAds(const std::string & category, ads::OnGetAdsCallback callback) override;
  const std::string GetAdsLocale() const override;
  uint64_t GetAdsPerDay() const override;
  uint64_t GetAdsPerHour() const override;
  void GetClientInfo(ads::ClientInfo * info) const override;
  const std::vector<std::string> GetLocales() const override;
  bool IsAdsEnabled() const override;
  bool IsForeground() const override;
  bool IsNetworkConnectionAvailable() override;
  bool IsNotificationsAvailable() const override;
  void KillTimer(uint32_t timer_id) override;
  void Load(const std::string & name, ads::OnLoadCallback callback) override;
  const std::string LoadJsonSchema(const std::string & name) override;
  void LoadSampleBundle(ads::OnLoadSampleBundleCallback callback) override;
  void LoadUserModelForLocale(const std::string & locale, ads::OnLoadCallback callback) const override;
  std::unique_ptr<ads::LogStream> Log(const char * file, const int line, const ads::LogLevel log_level) const override;
  void Reset(const std::string & name, ads::OnResetCallback callback) override;
  void Save(const std::string & name, const std::string & value, ads::OnSaveCallback callback) override;
  void SaveBundleState(std::unique_ptr<ads::BundleState> state, ads::OnSaveCallback callback) override;
  void SetCatalogIssuers(std::unique_ptr<ads::IssuersInfo> info) override;
  void SetIdleThreshold(const int threshold) override;
  uint32_t SetTimer(const uint64_t time_offset) override;
  void ShowNotification(std::unique_ptr<ads::NotificationInfo> info) override;
  void URLRequest(const std::string & url, const std::vector<std::string> & headers, const std::string & content, const std::string & content_type, const ads::URLRequestMethod method, ads::URLRequestCallback callback) override;
};
