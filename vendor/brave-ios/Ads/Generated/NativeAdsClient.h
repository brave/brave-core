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

  bool IsNetworkConnectionAvailable() const override;
  bool IsForeground() const override;
  bool IsFullScreen() const override;
  bool CanShowBackgroundNotifications() const override;
  void ShowNotification(const ads::AdNotificationInfo& info) override;
  bool ShouldShowNotifications() override;
  void CloseNotification(const std::string & uuid) override;
  void RecordAdEvent(const std::string& ad_type,
                     const std::string& confirmation_type,
                     const uint64_t timestamp) const override;
  std::vector<uint64_t> GetAdEvents(
      const std::string& ad_type,
      const std::string& confirmation_type) const override;
  void UrlRequest(ads::UrlRequestPtr url_request, ads::UrlRequestCallback callback) override;
  void Save(const std::string & name, const std::string & value, ads::ResultCallback callback) override;
  void Load(const std::string & name, ads::LoadCallback callback) override;
  void LoadAdsResource(const std::string& id,
                       const int version,
                       ads::LoadCallback callback) override;
  void GetBrowsingHistory(const int max_count,
                          const int days_ago,
                          ads::GetBrowsingHistoryCallback callback) override;
  std::string LoadResourceForId(const std::string & id) override;
  void Log(const char * file, const int line, const int verbose_level, const std::string & message) override;
  void RunDBTransaction(ads::DBTransactionPtr transaction, ads::RunDBTransactionCallback callback) override;
  void OnAdRewardsChanged() override;
  void SetBooleanPref(const std::string & path, const bool value) override;
  bool GetBooleanPref(const std::string & path) const override;
  void SetIntegerPref(const std::string & path, const int value) override;
  int GetIntegerPref(const std::string & path) const override;
  void SetDoublePref(const std::string & path, const double value) override;
  double GetDoublePref(const std::string & path) const override;
  void SetStringPref(const std::string & path, const std::string& value) override;
  std::string GetStringPref(const std::string & path) const override;
  void SetInt64Pref(const std::string & path, const int64_t value) override;
  int64_t GetInt64Pref(const std::string & path) const override;
  void SetUint64Pref(const std::string & path, const uint64_t value) override;
  uint64_t GetUint64Pref(const std::string & path) const override;
  void ClearPref(const std::string & path) override;
  void RecordP2AEvent(const std::string& name, const ads::P2AEventType type, const std::string& value) override;
};
