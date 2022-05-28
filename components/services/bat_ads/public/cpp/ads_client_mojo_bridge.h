/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_

#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "bat/ads/ads_client.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"

class GURL;

namespace base {
class Time;
}  // namespace base

namespace bat_ads {

class AdsClientMojoBridge
    : public mojom::BatAdsClient,
      public base::SupportsWeakPtr<AdsClientMojoBridge> {
 public:
  explicit AdsClientMojoBridge(
      ads::AdsClient* ads_client);

  ~AdsClientMojoBridge() override;

  AdsClientMojoBridge(const AdsClientMojoBridge&) = delete;
  AdsClientMojoBridge& operator=(const AdsClientMojoBridge&) = delete;

  // Overridden from BatAdsClient:
  bool IsBrowserActive(bool* out_is_browser_active) override;
  void IsBrowserActive(IsBrowserActiveCallback callback) override;
  bool IsBrowserInFullScreenMode(
      bool* out_is_browser_in_full_screen_mosw) override;
  void IsBrowserInFullScreenMode(
      IsBrowserInFullScreenModeCallback callback) override;
  bool IsNetworkConnectionAvailable(bool* out_available) override;
  void IsNetworkConnectionAvailable(
      IsNetworkConnectionAvailableCallback callback) override;
  bool CanShowBackgroundNotifications(bool* out_can_show) override;
  void CanShowBackgroundNotifications(
      CanShowBackgroundNotificationsCallback callback) override;
  bool ShouldShowNotifications(bool* out_should_show) override;
  void ShouldShowNotifications(
      ShouldShowNotificationsCallback callback) override;
  bool GetAdEvents(const std::string& ad_type,
                   const std::string& confirmation_type,
                   std::vector<base::Time>* out_ad_events) override;
  void GetAdEvents(const std::string& ad_type,
                   const std::string& confirmation_type,
                   GetAdEventsCallback callback) override;

  bool LoadDataResource(const std::string& name,
                        std::string* out_value) override;
  void LoadDataResource(const std::string& name,
                        LoadDataResourceCallback callback) override;
  void ClearScheduledCaptcha() override;
  void GetScheduledCaptcha(const std::string& payment_id,
                           GetScheduledCaptchaCallback callback) override;
  void ShowScheduledCaptchaNotification(const std::string& payment_id,
                                        const std::string& captcha_id) override;
  void Log(const std::string& file,
           const int32_t line,
           const int32_t verbose_level,
           const std::string& message) override;

  void LoadFileResource(const std::string& id,
                        const int version,
                        LoadFileResourceCallback callback) override;

  void GetBrowsingHistory(const int max_count,
                          const int days_ago,
                          GetBrowsingHistoryCallback callback) override;

  void RecordP2AEvent(const std::string& name,
                      const ads::mojom::P2AEventType type,
                      const std::string& out_value) override;

  void LogTrainingInstance(
      brave_federated::mojom::TrainingInstancePtr training_instance) override;

  void Load(
      const std::string& name,
      LoadCallback callback) override;
  void Save(
      const std::string& name,
      const std::string& value,
      SaveCallback callback) override;
  void UrlRequest(ads::mojom::UrlRequestPtr url_request,
                  UrlRequestCallback callback) override;
  void ShowNotification(
      const std::string& json) override;
  void CloseNotification(
      const std::string& uuid) override;

  void RecordAdEventForId(const std::string& id,
                          const std::string& ad_type,
                          const std::string& confirmation_type,
                          const base::Time time) override;
  void ResetAdEventsForId(const std::string& id) override;

  void RunDBTransaction(ads::mojom::DBTransactionPtr transaction,
                        RunDBTransactionCallback callback) override;
  void OnAdRewardsChanged() override;

  void GetBooleanPref(
      const std::string& path,
      GetBooleanPrefCallback callback) override;
  void SetBooleanPref(
      const std::string& path,
      const bool value) override;
  void GetIntegerPref(
      const std::string& path,
      GetIntegerPrefCallback callback) override;
  void SetIntegerPref(
      const std::string& path,
      const int value) override;
  void GetDoublePref(
      const std::string& path,
      GetDoublePrefCallback callback) override;
  void SetDoublePref(
      const std::string& path,
      const double value) override;
  void GetStringPref(
      const std::string& path,
      GetStringPrefCallback callback) override;
  void SetStringPref(
      const std::string& path,
      const std::string& value) override;
  void GetInt64Pref(
      const std::string& path,
      GetInt64PrefCallback callback) override;
  void SetInt64Pref(
      const std::string& path,
      const int64_t value) override;
  void GetUint64Pref(
      const std::string& path,
      GetUint64PrefCallback callback) override;
  void SetUint64Pref(
      const std::string& path,
      const uint64_t value) override;
  void GetTimePref(const std::string& path,
                   GetTimePrefCallback callback) override;
  void SetTimePref(const std::string& path, const base::Time value) override;
  void ClearPref(
      const std::string& path) override;
  void HasPrefPath(const std::string& path,
                   HasPrefPathCallback callback) override;

 private:
  // workaround to pass base::OnceCallback into std::bind
  template <typename Callback>
  class CallbackHolder {
   public:
    CallbackHolder(
        base::WeakPtr<AdsClientMojoBridge> client,
        Callback callback)
        : client_(client),
          callback_(std::move(callback)) {}

    ~CallbackHolder() = default;

    bool is_valid() {
      return !!client_.get();
    }

    Callback& get() {
      return callback_;
    }

   private:
    base::WeakPtr<AdsClientMojoBridge> client_;
    Callback callback_;
  };

  static void OnGetBrowsingHistory(
      CallbackHolder<GetBrowsingHistoryCallback>* holder,
      const std::vector<GURL>& history);

  static void OnLoad(CallbackHolder<LoadCallback>* holder,
                     const bool success,
                     const std::string& value);

  static void OnSave(CallbackHolder<SaveCallback>* holder, const bool success);

  static void OnURLRequest(CallbackHolder<UrlRequestCallback>* holder,
                           const ads::mojom::UrlResponse& url_response);

  static void OnGetScheduledCaptcha(
      CallbackHolder<GetScheduledCaptchaCallback>* holder,
      const std::string& captcha_id);

  static void OnRunDBTransaction(
      CallbackHolder<RunDBTransactionCallback>* holder,
      ads::mojom::DBCommandResponsePtr response);

  raw_ptr<ads::AdsClient> ads_client_ = nullptr;  // NOT OWNED
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_ADS_CLIENT_MOJO_BRIDGE_H_
