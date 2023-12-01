/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/client/ads_client.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom-forward.h"
#include "brave/components/services/bat_ads/bat_ads_client_notifier_impl.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {
class AdsClientNotifierObserver;
struct NotificationAdInfo;
}  // namespace brave_ads

namespace bat_ads {

class BatAdsClientMojoBridge : public brave_ads::AdsClient {
 public:
  explicit BatAdsClientMojoBridge(
      mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info,
      mojo::PendingReceiver<mojom::BatAdsClientNotifier> client_notifier);

  BatAdsClientMojoBridge(const BatAdsClientMojoBridge&) = delete;
  BatAdsClientMojoBridge& operator=(const BatAdsClientMojoBridge&) = delete;

  BatAdsClientMojoBridge(BatAdsClientMojoBridge&& other) noexcept = delete;
  BatAdsClientMojoBridge& operator=(BatAdsClientMojoBridge&& other) noexcept =
      delete;

  ~BatAdsClientMojoBridge() override;

  // AdsClient:
  void AddObserver(brave_ads::AdsClientNotifierObserver* observer) override;
  void RemoveObserver(brave_ads::AdsClientNotifierObserver* observer) override;
  void NotifyPendingObservers() override;

  bool IsNetworkConnectionAvailable() const override;

  bool IsBrowserActive() const override;
  bool IsBrowserInFullScreenMode() const override;

  bool CanShowNotificationAds() override;
  bool CanShowNotificationAdsWhileBrowserIsBackgrounded() const override;
  void ShowNotificationAd(const brave_ads::NotificationAdInfo& ad) override;
  void CloseNotificationAd(const std::string& placement_id) override;

  void CacheAdEventForInstanceId(const std::string& id,
                                 const std::string& ad_type,
                                 const std::string& confirmation_type,
                                 base::Time time) const override;
  std::vector<base::Time> GetCachedAdEvents(
      const std::string& ad_type,
      const std::string& confirmation_type) const override;
  void ResetAdEventCacheForInstanceId(const std::string& id) const override;

  void GetBrowsingHistory(
      int max_count,
      int days_ago,
      brave_ads::GetBrowsingHistoryCallback callback) override;

  void UrlRequest(brave_ads::mojom::UrlRequestInfoPtr url_request,
                  brave_ads::UrlRequestCallback callback) override;

  void Save(const std::string& name,
            const std::string& value,
            brave_ads::SaveCallback callback) override;
  void Load(const std::string& name, brave_ads::LoadCallback callback) override;

  void LoadComponentResource(const std::string& id,
                             int version,
                             brave_ads::LoadFileCallback callback) override;

  std::string LoadDataResource(const std::string& name) override;

  void GetScheduledCaptcha(
      const std::string& payment_id,
      brave_ads::GetScheduledCaptchaCallback callback) override;
  void ShowScheduledCaptchaNotification(const std::string& payment_id,
                                        const std::string& captcha_id) override;

  void RunDBTransaction(brave_ads::mojom::DBTransactionInfoPtr transaction,
                        brave_ads::RunDBTransactionCallback callback) override;

  void RecordP2AEvents(const std::vector<std::string>& events) override;

  void AddFederatedLearningPredictorTrainingSample(
      std::vector<brave_federated::mojom::CovariateInfoPtr> training_sample)
      override;

  std::optional<base::Value> GetProfilePref(const std::string& path) override;
  void SetProfilePref(const std::string& path, base::Value value) override;
  void ClearProfilePref(const std::string& path) override;
  bool HasProfilePrefPath(const std::string& path) const override;

  std::optional<base::Value> GetLocalStatePref(
      const std::string& path) override;
  void SetLocalStatePref(const std::string& path, base::Value value) override;
  void ClearLocalStatePref(const std::string& path) override;
  bool HasLocalStatePrefPath(const std::string& path) const override;

  void Log(const char* file,
           int line,
           int verbose_level,
           const std::string& message) override;

 private:
  std::optional<base::Value> CachedProfilePrefValue(
      const std::string& path) const;
  std::optional<base::Value> CachedLocalStatePrefValue(
      const std::string& path) const;

  base::flat_map</*path=*/std::string, /*value=*/base::Value>
      cached_profile_prefs_;
  base::flat_map</*path=*/std::string, /*value=*/base::Value>
      cached_local_state_prefs_;
  mojo::AssociatedRemote<mojom::BatAdsClient>
      bat_ads_client_associated_receiver_;
  BatAdsClientNotifierImpl notifier_impl_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_CLIENT_MOJO_BRIDGE_H_
