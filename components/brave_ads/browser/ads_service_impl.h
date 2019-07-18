/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "bat/ads/ads_client.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/background_helper.h"
#include "brave/components/brave_ads/browser/notification_helper.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "components/history/core/browser/history_service_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

#if !defined(OS_ANDROID)
#include "ui/base/idle/idle.h"
#endif

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
  explicit AdsServiceImpl(Profile* profile);
  ~AdsServiceImpl() override;

  // AdsService implementation
  bool IsSupportedRegion() const override;

  void SetAdsEnabled(const bool is_enabled) override;

  void SetAdsPerHour(const uint64_t ads_per_hour) override;

  void SetConfirmationsIsReady(const bool is_ready) override;
  void ChangeLocale(const std::string& locale) override;
  void ClassifyPage(const std::string& url, const std::string& page) override;
  void OnMediaStart(SessionID tab_id) override;
  void OnMediaStop(SessionID tab_id) override;
  void OnTabUpdated(
      SessionID tab_id,
      const GURL& url,
      const bool is_active) override;
  void OnTabClosed(SessionID tab_id) override;

  void Shutdown() override;

  // AdsClient implementation
  bool IsAdsEnabled() const override;

  uint64_t GetAdsPerHour() const override;
  uint64_t GetAdsPerDay() const override;

 private:
  friend class AdsNotificationHandler;

  void Start();
  bool StartService();
  void UpdateIsProductionFlag();
  void UpdateIsDebugFlag();
  void UpdateIsTestingFlag();
  void Stop();
  void ResetTimer();
  void CheckIdleState();
#if !defined(OS_ANDROID)
  void ProcessIdleState(ui::IdleState idle_state);
#endif
  int GetIdleThreshold();
  void OnShow(Profile* profile, const std::string& notification_id);
  void OnClose(
      Profile* profile,
      const GURL& origin,
      const std::string& notification_id,
      bool by_user,
      base::OnceClosure completed_closure);
  void ViewAd(const std::string& id);
  void OnViewAd(const std::string& json);
  void OpenNewTabWithUrl(const std::string& url);

  // AdsClient implementation
  bool IsForeground() const override;
  const std::string GetAdsLocale() const override;
  void GetClientInfo(ads::ClientInfo* info) const override;
  const std::vector<std::string> GetLocales() const override;
  void ShowNotification(std::unique_ptr<ads::NotificationInfo> info) override;
  void CloseNotification(const std::string& id) override;
  void SetCatalogIssuers(std::unique_ptr<ads::IssuersInfo> info) override;
  void ConfirmAd(std::unique_ptr<ads::NotificationInfo> info) override;
  uint32_t SetTimer(const uint64_t time_offset) override;
  void KillTimer(uint32_t timer_id) override;
  void URLRequest(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      ads::URLRequestMethod method,
      ads::URLRequestCallback callback) override;
  void Save(
      const std::string& name,
      const std::string& value,
      ads::OnSaveCallback callback) override;
  void Load(
      const std::string& name,
      ads::OnLoadCallback callback) override;
  void SaveBundleState(
      std::unique_ptr<ads::BundleState> bundle_state,
      ads::OnSaveCallback callback) override;
  const std::string LoadJsonSchema(const std::string& name) override;
  void Reset(
      const std::string& name,
      ads::OnResetCallback callback) override;
  void GetAds(
      const std::string& category,
      ads::OnGetAdsCallback callback) override;
  void LoadSampleBundle(ads::OnLoadSampleBundleCallback callback) override;
  void EventLog(const std::string& json) override;
  std::unique_ptr<ads::LogStream> Log(
      const char* file,
      int line,
      const ads::LogLevel log_level) const override;
  void SetIdleThreshold(const int threshold) override;
  bool IsNotificationsAvailable() const override;
  void LoadUserModelForLocale(
      const std::string& locale,
      ads::OnLoadCallback callback) const override;
  bool IsNetworkConnectionAvailable() override;
  void ResetTheWholeState(const base::Callback<void(bool)>& callback) override;

  // history::HistoryServiceObserver
  void OnURLsDeleted(
      history::HistoryService* history_service,
      const history::DeletionInfo& deletion_info) override;

  // BackgroundHelper::Observer impl
  void OnBackground() override;
  void OnForeground() override;

  void OnURLLoaderComplete(network::SimpleURLLoader* loader,
                           ads::URLRequestCallback callback,
                           std::unique_ptr<std::string> response_body);

  void OnGetAdsForCategory(
      const ads::OnGetAdsCallback& callback,
      const std::string& category,
      const std::vector<ads::AdInfo>& ads);

  void OnSaveBundleState(const ads::OnSaveCallback& callback, bool success);
  void OnLoaded(
      const ads::OnLoadCallback& callback,
      const std::string& value);
  void OnSaved(const ads::OnSaveCallback& callback, bool success);
  void OnReset(const ads::OnResetCallback& callback, bool success);
  void OnResetTheWholeState(base::Callback<void(bool)> callback,
                                 bool success);
  void OnTimer(uint32_t timer_id);

  void MigratePrefs() const;
  bool MigratePrefs(
      const int source_version,
      const int dest_version,
      const bool is_dry_run = false) const;
  void MigratePrefsVersion1To2() const;
  void MigratePrefsVersion2To3() const;
  int GetPrefsVersion() const;
  void OnPrefsChanged(const std::string& pref);

  void DisableAdsForUnsupportedRegion(
    const std::string& region,
    const std::vector<std::string>& regions) const;
  void MayBeShowFirstLaunchNotificationForSupportedRegion(
    const std::string& region,
    const std::vector<std::string>& regions) const;

  void OnCreate();
  void OnInitialize(const int32_t result);
  void ShutdownBatAds();
  void OnShutdownBatAds(const int32_t result);
  void ResetAllState();
  void OnResetAllState(bool success);
  void EnsureBaseDirectoryExists();
  void OnEnsureBaseDirectoryExists(bool success);
  void OnRemoveAllHistory(const int32_t result);
  void MaybeStart(bool should_restart);
  void NotificationTimedOut(
      uint32_t timer_id,
      const std::string& notification_id);
  void MaybeViewAd();
  void RetryViewingAdWithId(const std::string& id);
  bool ShouldShowMyFirstAdNotification();
  void MaybeShowMyFirstAdNotification();
  void MaybeShowFirstLaunchNotification();
  bool ShouldShowFirstLaunchNotification();
  void ShowFirstLaunchNotification();
  void MaybeStartFirstLaunchNotificationTimeoutTimer();
  void StartFirstLaunchNotificationTimeoutTimer();
  void OnFirstLaunchNotificationTimedOut(uint32_t timer_id);
  uint64_t GetFirstLaunchNotificationTimeoutTimerOffset();
  bool HasFirstLaunchNotificationExpired();
  uint64_t GetFirstLaunchNotificationTimeout();
  void RemoveFirstLaunchNotification();

  uint32_t next_timer_id();

  // are we still connected to the ads lib
  bool connected();

  Profile* profile_;  // NOT OWNED
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  const base::FilePath base_path_;
  std::map<uint32_t, std::unique_ptr<base::OneShotTimer>> timers_;
  bool is_initialized_;
  std::string retry_viewing_ad_with_id_;
  uint32_t next_timer_id_;
  uint32_t ads_launch_id_;
  std::unique_ptr<BundleStateDatabase> bundle_state_backend_;
  NotificationDisplayService* display_service_;  // NOT OWNED
  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED

#if !defined(OS_ANDROID)
  ui::IdleState last_idle_state_;
  bool is_foreground_;
#endif

  base::RepeatingTimer idle_poll_timer_;

  PrefChangeRegistrar profile_pref_change_registrar_;

  mojo::AssociatedBinding<bat_ads::mojom::BatAdsClient> bat_ads_client_binding_;
  bat_ads::mojom::BatAdsAssociatedPtr bat_ads_;
  bat_ads::mojom::BatAdsServicePtr bat_ads_service_;

  base::flat_set<network::SimpleURLLoader*> url_loaders_;

  DISALLOW_COPY_AND_ASSIGN(AdsServiceImpl);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_
