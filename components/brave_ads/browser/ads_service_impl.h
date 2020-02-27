/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_

#include <stdint.h>
#include <deque>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "bat/ads/ads.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/mojom.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/background_helper.h"
#include "brave/components/brave_ads/browser/notification_helper.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "components/history/core/browser/history_service_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/base/idle/idle.h"

using brave_rewards::RewardsNotificationService;

class NotificationDisplayService;
class Profile;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_rewards {
class RewardsService;
}  // namespace brave_rewards

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace brave_ads {

class AdsNotificationHandler;
class BundleStateDatabase;

class AdsServiceImpl : public AdsService,
                       public ads::AdsClient,
                       public history::HistoryServiceObserver,
                       BackgroundHelper::Observer,
                       public base::SupportsWeakPtr<AdsServiceImpl> {
 public:
  // AdsService implementation
  explicit AdsServiceImpl(Profile* profile);
  ~AdsServiceImpl() override;

  bool IsSupportedLocale() const override;
  bool IsNewlySupportedLocale() override;

  void SetEnabled(
      const bool is_enabled) override;

  void SetAllowAdConversionTracking(
      const bool should_allow) override;

  void SetAdsPerHour(
      const uint64_t ads_per_hour) override;

  void SetConfirmationsIsReady(
      const bool is_ready) override;

  void ChangeLocale(
      const std::string& locale) override;

  void OnPageLoaded(
      const std::string& url,
      const std::string& html) override;

  void OnMediaStart(
      const SessionID& tab_id) override;
  void OnMediaStop(
      const SessionID& tab_id) override;

  void OnTabUpdated(
      const SessionID& tab_id,
      const GURL& url,
      const bool is_active) override;
  void OnTabClosed(
      const SessionID& tab_id) override;

  void GetAdsHistory(
      const uint64_t from_timestamp,
      const uint64_t to_timestamp,
      OnGetAdsHistoryCallback callback) override;

  void ToggleAdThumbUp(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const int action,
      OnToggleAdThumbUpCallback callback) override;
  void ToggleAdThumbDown(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const int action,
      OnToggleAdThumbDownCallback callback) override;
  void ToggleAdOptInAction(
      const std::string& category,
      const int action,
      const OnToggleAdOptInActionCallback callback) override;
  void ToggleAdOptOutAction(
      const std::string& category,
      const int action,
      OnToggleAdOptOutActionCallback callback) override;
  void ToggleSaveAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool saved,
      OnToggleSaveAdCallback callback) override;
  void ToggleFlagAd(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const bool flagged,
      OnToggleFlagAdCallback callback) override;

  // AdsClient implementation
  bool IsEnabled() const override;

  bool ShouldAllowAdConversionTracking() const override;

  uint64_t GetAdsPerHour() const override;
  uint64_t GetAdsPerDay() const override;

  // KeyedService implementation
  void Shutdown() override;

 private:
  // AdsService implementation
  friend class AdsNotificationHandler;

  void OnCreate();

  void OnInitialize(
      const int32_t result);

  void ShutdownBatAds();
  void OnShutdownBatAds(
      const int32_t result);

  bool StartService();

  void MaybeStart(
      const bool should_restart);
  void Start();
  void Stop();

  void ResetAllState();
  void OnResetAllState(
      const bool success);

  void EnsureBaseDirectoryExists();
  void OnEnsureBaseDirectoryExists(
      const bool success);

  void SetEnvironment();

  void UpdateIsDebugFlag();
  bool IsDebug() const;
  void UpdateIsTestingFlag();
  bool IsTesting() const;

  void StartCheckIdleStateTimer();
  void CheckIdleState();
  void ProcessIdleState(
      const ui::IdleState idle_state);
  int GetIdleThreshold();

  void OnShow(
      Profile* profile,
      const std::string& uuid);
  void OnClose(
      Profile* profile,
      const GURL& origin,
      const std::string& uuid,
      const bool by_user,
      base::OnceClosure completed_closure);

  void MaybeViewAdNotification();
  void ViewAdNotification(
      const std::string& uuid);
  void OnViewAdNotification(
      const std::string& json);
  void RetryViewingAdNotification(
      const std::string& uuid);
  void ResetTheWholeState(const base::Callback<void(bool)>& callback) override;

  void SetAdsServiceForNotificationHandler();
  void ClearAdsServiceForNotificationHandler();

  void OpenNewTabWithUrl(
      const std::string& url);

  void NotificationTimedOut(
      const uint32_t timer_id,
      const std::string& uuid);

  void OnURLLoaderComplete(
      network::SimpleURLLoader* loader,
      ads::URLRequestCallback callback,
      const std::unique_ptr<std::string> response_body);

  void OnGetCreativeAdNotifications(
      const ads::OnGetCreativeAdNotificationsCallback& callback,
      const std::vector<std::string>& categories,
      const ads::CreativeAdNotificationList& ads);

  void OnGetAdConversions(
      const ads::OnGetAdConversionsCallback& callback,
      const std::string& url,
      const ads::AdConversionList& ad_conversions);

  void OnGetAdsHistory(
      OnGetAdsHistoryCallback callback,
      const std::string& json);

  void OnRemoveAllHistory(
      const int32_t result);

  void OnToggleAdThumbUp(
      OnToggleAdThumbUpCallback callback,
      const std::string& creative_instance_id,
      const int action);
  void OnToggleAdThumbDown(
      const OnToggleAdThumbDownCallback callback,
      const std::string& creative_instance_id,
      const int action);
  void OnToggleAdOptInAction(
      OnToggleAdOptInActionCallback callback,
      const std::string& category,
      const int action);
  void OnToggleAdOptOutAction(
      OnToggleAdOptOutActionCallback callback,
      const std::string& category,
      const int action);
  void OnToggleSaveAd(
      OnToggleSaveAdCallback callback,
      const std::string& creative_instance_id,
      const bool saved);
  void OnToggleFlagAd(
      OnToggleSaveAdCallback callback,
      const std::string& creative_instance_id,
      const bool flagged);

  void OnSaveBundleState(
      const ads::OnSaveCallback& callback,
      const bool success);
  void OnLoaded(
      const ads::OnLoadCallback& callback,
      const std::string& value);
  void OnSaved(
      const ads::OnSaveCallback& callback,
      const bool success);
  void OnReset(
      const ads::OnResetCallback& callback,
      const bool success);

  void OnTimer(
      const uint32_t timer_id);
  void OnResetTheWholeState(base::Callback<void(bool)> callback,
                                 bool success);

  void MigratePrefs();
  bool MigratePrefs(
      const int source_version,
      const int dest_version,
      const bool is_dry_run = false);
  void MigratePrefsVersion1To2();
  void MigratePrefsVersion2To3();
  void MigratePrefsVersion3To4();
  void MigratePrefsVersion4To5();
  void MigratePrefsVersion5To6();
  void MigratePrefsVersion6To7();
  int GetPrefsVersion() const;

  bool IsUpgradingFromPreBraveAdsBuild();

  void DisableAdsIfUpgradingFromPreBraveAdsBuild();
  void DisableAdsForUnsupportedRegions(
      const std::string& region,
      const std::vector<std::string>& regions);
  void MayBeShowOnboardingForSupportedRegion(
      const std::string& region,
      const std::vector<std::string>& regions);
  uint64_t MigrateTimestampToDoubleT(
      const uint64_t timestamp_in_seconds) const;

  void MaybeShowOnboarding();
  bool ShouldShowOnboarding();
  void ShowOnboarding();
  void RemoveOnboarding();
  void MaybeStartRemoveOnboardingTimer();
  bool ShouldRemoveOnboarding() const;
  void StartRemoveOnboardingTimer();
  void OnRemoveOnboarding(
      const uint32_t timer_id);

  void MaybeShowMyFirstAdNotification();
  bool ShouldShowMyFirstAdNotification() const;

  bool GetBooleanPref(
      const std::string& path) const;
  void SetBooleanPref(
      const std::string& path,
      const bool value);
  int GetIntegerPref(
      const std::string& path) const;
  void SetIntegerPref(
      const std::string& path,
      const int value);
  double GetDoublePref(
      const std::string& path) const;
  void SetDoublePref(
      const std::string& path,
      const double value);
  std::string GetStringPref(
      const std::string& path) const;
  void SetStringPref(
      const std::string& path,
      const std::string& value);
  int64_t GetInt64Pref(
      const std::string& path) const;
  void SetInt64Pref(
      const std::string& path,
      const int64_t value);
  uint64_t GetUint64Pref(
      const std::string& path) const;
  void SetUint64Pref(
      const std::string& path,
      const uint64_t value);
  bool PrefExists(
      const std::string& path) const;
  void OnPrefsChanged(
      const std::string& pref);

  uint32_t next_timer_id();

  std::string LoadDataResourceAndDecompressIfNeeded(
      const int id) const;

  bool connected();

  // AdsClient implementation
  void GetClientInfo(
      ads::ClientInfo* info) const override;

  std::string GetLocale() const override;

  bool IsNetworkConnectionAvailable() const override;

  void SetIdleThreshold(
      const int threshold) override;

  bool IsForeground() const override;

  std::vector<std::string> GetUserModelLanguages() const override;
  void LoadUserModelForLanguage(
      const std::string& language,
      ads::OnLoadCallback callback) const override;

  void ShowNotification(
      std::unique_ptr<ads::AdNotificationInfo> info) override;
  bool ShouldShowNotifications() override;
  void CloseNotification(
      const std::string& uuid) override;

  void SetCatalogIssuers(
      std::unique_ptr<ads::IssuersInfo> info) override;

  void ConfirmAdNotification(
      std::unique_ptr<ads::AdNotificationInfo> info) override;
  void ConfirmAction(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const ads::ConfirmationType& confirmation_type) override;

  uint32_t SetTimer(
      const uint64_t time_offset) override;
  void KillTimer(
      const uint32_t timer_id) override;

  bool CanShowBackgroundNotifications() const override;

  void URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const ads::URLRequestMethod method,
      ads::URLRequestCallback callback) override;

  void Save(
      const std::string& name,
      const std::string& value,
      ads::OnSaveCallback callback) override;
  void Load(
      const std::string& name,
      ads::OnLoadCallback callback) override;
  void Reset(
      const std::string& name,
      ads::OnResetCallback callback) override;

  std::string LoadJsonSchema(
      const std::string& name) override;

  void LoadSampleBundle(
      ads::OnLoadSampleBundleCallback callback) override;

  void SaveBundleState(
      std::unique_ptr<ads::BundleState> bundle_state,
      ads::OnSaveCallback callback) override;

  void GetCreativeAdNotifications(
      const std::vector<std::string>& categories,
      ads::OnGetCreativeAdNotificationsCallback callback) override;

  void GetAdConversions(
      const std::string& url,
      ads::OnGetAdConversionsCallback callback) override;

  void EventLog(
      const std::string& json) const override;

  std::unique_ptr<ads::LogStream> Log(
      const char* file,
      const int line,
      const ads::LogLevel log_level) const override;

  // history::HistoryServiceObserver
  void OnURLsDeleted(
      history::HistoryService* history_service,
      const history::DeletionInfo& deletion_info) override;

  // BackgroundHelper::Observer implementation
  void OnBackground() override;
  void OnForeground() override;

///////////////////////////////////////////////////////////////////////////////

  Profile* profile_;  // NOT OWNED

  bool is_initialized_;

  bool is_upgrading_from_pre_brave_ads_build_;

  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  const base::FilePath base_path_;

  std::map<uint32_t, std::unique_ptr<base::OneShotTimer>> timers_;
  uint32_t next_timer_id_;

  std::string retry_viewing_ad_notification_with_uuid_;

  uint32_t remove_onboarding_timer_id_;

  ui::IdleState last_idle_state_;

  base::RepeatingTimer idle_poll_timer_;

  PrefChangeRegistrar profile_pref_change_registrar_;

  base::flat_set<network::SimpleURLLoader*> url_loaders_;

  std::unique_ptr<BundleStateDatabase> bundle_state_backend_;

  NotificationDisplayService* display_service_;  // NOT OWNED
  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED

  mojo::AssociatedReceiver<bat_ads::mojom::BatAdsClient>
      bat_ads_client_receiver_;
  mojo::AssociatedRemote<bat_ads::mojom::BatAds> bat_ads_;
  mojo::Remote<bat_ads::mojom::BatAdsService> bat_ads_service_;

  DISALLOW_COPY_AND_ASSIGN(AdsServiceImpl);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_
