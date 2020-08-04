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

  bool IsEnabled() const override;
  bool ShouldAllowAdConversionTracking() const override;
  uint64_t GetAdsPerDay() const override;
  uint64_t GetAdsPerHour() const override;
  bool IsNetworkConnectionAvailable() const override;
  void SetIdleThreshold(const int threshold) override;
  bool IsForeground() const override;
  bool CanShowBackgroundNotifications() const override;
  void ShowNotification(std::unique_ptr<ads::AdNotificationInfo> info) override;
  bool ShouldShowNotifications() override;
  void CloseNotification(const std::string & uuid) override;
  void UrlRequest(ads::UrlRequestPtr url_request, ads::UrlRequestCallback callback) override;
  void Save(const std::string & name, const std::string & value, ads::ResultCallback callback) override;
  void Load(const std::string & name, ads::LoadCallback callback) override;
  void LoadUserModelForId(const std::string & id, ads::LoadCallback callback) override;
  std::string LoadResourceForId(const std::string & id) override;
  void Log(const char * file, const int line, const int verbose_level, const std::string & message) override;
  bool ShouldAllowAdsSubdivisionTargeting() const override;
  void SetAllowAdsSubdivisionTargeting(const bool should_allow) override;
  std::string GetAdsSubdivisionTargetingCode() const override;
  void SetAdsSubdivisionTargetingCode(const std::string & subdivision_targeting_code) override;
  std::string GetAutomaticallyDetectedAdsSubdivisionTargetingCode() const override;
  void SetAutomaticallyDetectedAdsSubdivisionTargetingCode(const std::string & subdivision_targeting_code) override;
  void RunDBTransaction(ads::DBTransactionPtr transaction, ads::RunDBTransactionCallback callback) override;
  void OnAdRewardsChanged() override;
};
