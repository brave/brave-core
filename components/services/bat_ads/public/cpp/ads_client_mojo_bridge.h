/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "bat/ads/ads_client.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace bat_ads {

class AdsClientMojoBridge : public mojom::BatAdsClient,
                         public base::SupportsWeakPtr<AdsClientMojoBridge> {
 public:
  explicit AdsClientMojoBridge(ads::AdsClient* ads_client);
  ~AdsClientMojoBridge() override;

  // Overridden from BatAdsClient:
  bool IsAdsEnabled(bool* out_is_enabled) override;
  void IsAdsEnabled(IsAdsEnabledCallback callback) override;
  bool IsForeground(bool* out_is_foreground) override;
  void IsForeground(IsForegroundCallback callback) override;
  bool GetAdsLocale(std::string* out_locale) override;
  void GetAdsLocale(GetAdsLocaleCallback callback) override;
  bool GetLocales(std::vector<std::string>* out_locales) override;
  void GetLocales(GetLocalesCallback callback) override;
  bool GetAdsPerHour(uint64_t* out_ads_per_hour) override;
  void GetAdsPerHour(GetAdsPerHourCallback callback) override;
  bool GetAdsPerDay(uint64_t* out_ads_per_day) override;
  void GetAdsPerDay(GetAdsPerDayCallback callback) override;
  bool IsNetworkConnectionAvailable(bool* out_available) override;
  void IsNetworkConnectionAvailable(
      IsNetworkConnectionAvailableCallback callback) override;
  bool GenerateUUID(std::string* out_uuid) override;
  void GenerateUUID(GenerateUUIDCallback callback) override;
  bool IsNotificationsAvailable(bool* out_available) override;
  void IsNotificationsAvailable(
      IsNotificationsAvailableCallback callback) override;
  bool SetTimer(uint64_t time_offset, uint32_t* out_timer_id) override;
  void SetTimer(uint64_t time_offset, SetTimerCallback callback) override;
  bool LoadJsonSchema(const std::string& name, std::string* out_json) override;
  void LoadJsonSchema(const std::string& name,
                      LoadJsonSchemaCallback callback) override;
  bool GetClientInfo(const std::string& client_info,
                     std::string* out_client_info) override;
  void GetClientInfo(const std::string& client_info,
                     GetClientInfoCallback callback) override;

  void EventLog(const std::string& json) override;
  void SetIdleThreshold(int32_t threshold) override;
  void KillTimer(uint32_t timer_id) override;
  void Load(const std::string& name, LoadCallback callback) override;
  void Save(const std::string& name,
            const std::string& value,
            SaveCallback callback) override;
  void Reset(const std::string& name, ResetCallback callback) override;
  void LoadUserModelForLocale(const std::string& locale,
                              LoadUserModelForLocaleCallback callback) override;
  void URLRequest(const std::string& url,
                  const std::vector<std::string>& headers,
                  const std::string& content,
                  const std::string& content_type,
                  int32_t method,
                  URLRequestCallback callback) override;
  void LoadSampleBundle(LoadSampleBundleCallback callback) override;
  void ShowNotification(const std::string& notification_info) override;
  void SetCatalogIssuers(const std::string& issuers_info) override;
  void ConfirmAd(const std::string& notification_info) override;
  void SaveBundleState(const std::string& bundle_state,
                       SaveBundleStateCallback callback) override;
  void GetAds(const std::string& category,
              GetAdsCallback callback) override;

 private:
  // workaround to pass base::OnceCallback into std::bind
  template <typename Callback>
  class CallbackHolder {
   public:
    CallbackHolder(base::WeakPtr<AdsClientMojoBridge> client, Callback callback)
        : client_(client),
          callback_(std::move(callback)) {}
    ~CallbackHolder() = default;
    bool is_valid() { return !!client_.get(); }
    Callback& get() { return callback_; }

   private:
    base::WeakPtr<AdsClientMojoBridge> client_;
    Callback callback_;
  };

  static void OnLoad(CallbackHolder<LoadCallback>* holder,
                     ads::Result result,
                     const std::string& value);
  static void OnSave(CallbackHolder<SaveCallback>* holder,
                     ads::Result result);
  static void OnReset(CallbackHolder<ResetCallback>* holder,
                      ads::Result result);
  static void OnLoadUserModelForLocale(
      CallbackHolder<LoadUserModelForLocaleCallback>* holder,
      ads::Result result,
      const std::string& value);
  static void OnURLRequest(CallbackHolder<URLRequestCallback>* holder,
                           const int status_code,
                           const std::string& content,
                           const std::map<std::string, std::string>& headers);
  static void OnLoadSampleBundle(
      CallbackHolder<LoadSampleBundleCallback>* holder,
      ads::Result result,
      const std::string& value);
  static void OnSaveBundleState(
      CallbackHolder<SaveBundleStateCallback>* holder,
      ads::Result result);
  static void OnGetAds(
      CallbackHolder<GetAdsCallback>* holder,
      ads::Result result,
      const std::string& category,
      const std::vector<ads::AdInfo>& ad_info);


  ads::AdsClient* ads_client_;

  DISALLOW_COPY_AND_ASSIGN(AdsClientMojoBridge);
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_
