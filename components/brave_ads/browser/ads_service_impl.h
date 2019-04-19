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

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "bat/ads/ads_client.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/background_helper.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "components/history/core/browser/history_service_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "net/url_request/url_fetcher_delegate.h"

#if !defined(OS_ANDROID)
#include "ui/base/idle/idle.h"
#endif

class NotificationDisplayService;
class Profile;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_rewards {
class RewardsService;
}  // namespace brave_rewards

namespace brave_ads {

class AdsNotificationHandler;
class BundleStateDatabase;

class AdsServiceImpl : public AdsService,
                       public ads::AdsClient,
                       public net::URLFetcherDelegate,
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

  void TabUpdated(
      SessionID tab_id,
      const GURL& url,
      const bool is_active) override;
  void TabClosed(SessionID tab_id) override;
  void OnMediaStart(SessionID tab_id) override;
  void OnMediaStop(SessionID tab_id) override;
  void ClassifyPage(const std::string& url, const std::string& page) override;
  void SetConfirmationsIsReady(const bool is_ready) override;

  void Shutdown() override;

  // AdsClient implementation
  bool IsAdsEnabled() const override;

  uint64_t GetAdsPerHour() const override;
  uint64_t GetAdsPerDay() const override;

 private:
  friend class AdsNotificationHandler;

  typedef std::map<std::string, std::unique_ptr<const ads::NotificationInfo>>
      NotificationInfoMap;

  void Start();
  bool StartService();
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
  void OpenSettings(
      Profile* profile,
      const GURL& origin,
      bool should_close);

  // AdsClient implementation
  bool IsForeground() const override;
  const std::string GetAdsLocale() const override;
  void GetClientInfo(ads::ClientInfo* info) const override;
  const std::vector<std::string> GetLocales() const override;
  const std::string GenerateUUID() const override;
  void ShowNotification(std::unique_ptr<ads::NotificationInfo> info) override;
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

  // history::HistoryServiceObserver
  void OnURLsDeleted(
      history::HistoryService* history_service,
      const history::DeletionInfo& deletion_info) override;

  // URLFetcherDelegate impl
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // BackgroundHelper::Observer impl
  void OnBackground() override;
  void OnForeground() override;

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
  void OnTimer(uint32_t timer_id);
  void MigratePrefs() const;
  bool MigratePrefs(
      const int source_version,
      const int dest_version,
      const bool is_dry_run = false) const;
  void MigratePrefsVersion1To2() const;
  int GetPrefsVersion() const;
  void OnPrefsChanged(const std::string& pref);
  void OnCreate();
  void OnInitialize();
  void MaybeStart(bool should_restart);
  void OnMaybeStartForRegion(
      bool should_restart,
      bool is_supported_region);
  void NotificationTimedOut(
      uint32_t timer_id,
      const std::string& notification_id);

  uint32_t next_timer_id();

  // are we still connected to the ads lib
  bool connected();

  Profile* profile_;  // NOT OWNED
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  const base::FilePath base_path_;
  std::map<uint32_t, std::unique_ptr<base::OneShotTimer>> timers_;
  uint32_t next_timer_id_;
  bool is_supported_region_;
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

  NotificationInfoMap notification_ids_;
  std::map<const net::URLFetcher*, ads::URLRequestCallback> fetchers_;

  DISALLOW_COPY_AND_ASSIGN(AdsServiceImpl);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_
