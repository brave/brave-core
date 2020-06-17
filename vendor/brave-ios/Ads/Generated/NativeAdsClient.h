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
  void GetClientInfo(ads::ClientInfo * info) const override;
  bool IsNetworkConnectionAvailable() const override;
  void SetIdleThreshold(const int threshold) override;
  bool IsForeground() const override;
  bool CanShowBackgroundNotifications() const override;
  std::vector<std::string> GetUserModelLanguages() const override;
  void LoadUserModelForLanguage(const std::string & language, ads::LoadCallback callback) const override;
  void ShowNotification(std::unique_ptr<ads::AdNotificationInfo> info) override;
  bool ShouldShowNotifications() override;
  void CloseNotification(const std::string & uuid) override;
  void SetCatalogIssuers(std::unique_ptr<ads::IssuersInfo> info) override;
  void ConfirmAd(const ads::AdInfo& info, const ads::ConfirmationType confirmation_type) override;
  void ConfirmAction(const std::string& creative_instance_id, const std::string& creative_set_id, const ads::ConfirmationType confirmation_type) override;
  void URLRequest(const std::string & url, const std::vector<std::string> & headers, const std::string & content, const std::string & content_type, const ads::URLRequestMethod method, ads::URLRequestCallback callback) override;
  void Save(const std::string & name, const std::string & value, ads::ResultCallback callback) override;
  void Load(const std::string & name, ads::LoadCallback callback) override;
  void Reset(const std::string & name, ads::ResultCallback callback) override;
  std::string LoadJsonSchema(const std::string & name) override;
  void Log(const char * file, const int line, const int verbose_level, const std::string & message) override;
  bool ShouldAllowAdsSubdivisionTargeting() const override;
  void SetAllowAdsSubdivisionTargeting(const bool should_allow) override;
  std::string GetAdsSubdivisionTargetingCode() const override;
  void SetAdsSubdivisionTargetingCode(const std::string & subdivision_targeting_code) override;
  std::string GetAutomaticallyDetectedAdsSubdivisionTargetingCode() const override;
  void SetAutomaticallyDetectedAdsSubdivisionTargetingCode(const std::string & subdivision_targeting_code) override;
  void RunDBTransaction(ads::DBTransactionPtr transaction, ads::RunDBTransactionCallback callback) override;
};
