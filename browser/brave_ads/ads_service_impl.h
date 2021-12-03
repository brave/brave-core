/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_IMPL_H_

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/timer/timer.h"
#include "bat/ads/ads.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/database.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/browser/brave_ads/background_helper/background_helper.h"
#include "brave/components/brave_adaptive_captcha/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "components/history/core/browser/history_service_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "ui/base/idle/idle.h"

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#endif

using brave_ads::ResourceComponent;
using brave_rewards::RewardsNotificationService;

class NotificationDisplayService;
class Profile;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_rewards {
class RewardsService;
}  // namespace brave_rewards

namespace history {
class HistoryService;
}

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace brave_ads {

class AdsTooltipsDelegate;

class AdsServiceImpl : public AdsService,
                       public ads::AdsClient,
                       public history::HistoryServiceObserver,
                       BackgroundHelper::Observer,
                       public brave_ads::Observer,
                       public base::SupportsWeakPtr<AdsServiceImpl> {
 public:
  void OnWalletUpdated();

  // AdsService implementation
  explicit AdsServiceImpl(
      Profile* profile,
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
      brave_adaptive_captcha::BraveAdaptiveCaptchaService*
          adaptive_captcha_service,
      std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate,
#endif
      history::HistoryService* history_service);
  ~AdsServiceImpl() override;

  AdsServiceImpl(const AdsServiceImpl&) = delete;
  AdsServiceImpl& operator=(const AdsServiceImpl&) = delete;

  bool IsSupportedLocale() const override;

  bool IsEnabled() const override;
  void SetEnabled(const bool is_enabled) override;

  void SetAllowConversionTracking(const bool should_allow) override;

  int64_t GetAdsPerHour() const override;
  void SetAdsPerHour(const int64_t ads_per_hour) override;

  bool ShouldAllowAdsSubdivisionTargeting() const override;
  std::string GetAdsSubdivisionTargetingCode() const override;
  void SetAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) override;
  std::string GetAutoDetectedAdsSubdivisionTargetingCode() const override;
  void SetAutoDetectedAdsSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) override;

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  void ShowScheduledCaptcha(const std::string& payment_id,
                            const std::string& captcha_id) override;
  void SnoozeScheduledCaptcha() override;
#endif

  void OnShowAdNotification(const std::string& notification_id) override;
  void OnCloseAdNotification(const std::string& notification_id,
                             const bool by_user) override;
  void OnClickAdNotification(const std::string& notification_id) override;

  void ChangeLocale(const std::string& locale) override;

  void OnPrefChanged(const std::string& path);

  void OnHtmlLoaded(const SessionID& tab_id,
                    const std::vector<GURL>& redirect_chain,
                    const std::string& html) override;

  void OnTextLoaded(const SessionID& tab_id,
                    const std::vector<GURL>& redirect_chain,
                    const std::string& text) override;

  void OnUserGesture(const int32_t page_transition_type) override;

  void OnMediaStart(const SessionID& tab_id) override;
  void OnMediaStop(const SessionID& tab_id) override;

  void OnTabUpdated(const SessionID& tab_id,
                    const GURL& url,
                    const bool is_active,
                    const bool is_browser_active) override;

  void OnTabClosed(const SessionID& tab_id) override;

  void OnResourceComponentUpdated(const std::string& id) override;

  void OnNewTabPageAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const ads::mojom::NewTabPageAdEventType event_type) override;

  void OnPromotedContentAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const ads::mojom::PromotedContentAdEventType event_type) override;

  void GetInlineContentAd(const std::string& dimensions,
                          OnGetInlineContentAdCallback callback) override;

  void OnInlineContentAdEvent(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const ads::mojom::InlineContentAdEventType event_type) override;

  void PurgeOrphanedAdEventsForType(const ads::mojom::AdType ad_type) override;

  void GetAdsHistory(const double from_timestamp,
                     const double to_timestamp,
                     OnGetAdsHistoryCallback callback) override;

  void GetAccountStatement(GetAccountStatementCallback callback) override;

  void GetAdDiagnostics(GetAdDiagnosticsCallback callback) override;

  void ToggleAdThumbUp(const std::string& json,
                       OnToggleAdThumbUpCallback callback) override;
  void ToggleAdThumbDown(const std::string& json,
                         OnToggleAdThumbDownCallback callback) override;

  void ToggleAdOptIn(const std::string& category,
                     const int action,
                     const OnToggleAdOptInCallback callback) override;
  void ToggleAdOptOut(const std::string& category,
                      const int action,
                      OnToggleAdOptOutCallback callback) override;

  void ToggleSavedAd(const std::string& json,
                     OnToggleSavedAdCallback callback) override;

  void ToggleFlaggedAd(const std::string& json,
                       OnToggleFlaggedAdCallback callback) override;

  void ResetAllState(const bool should_shutdown) override;

  // KeyedService implementation
  void Shutdown() override;

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  void MaybeInitialize();
  void Initialize();
  void OnMigrateConfirmationsState(const bool success);

  void OnCreate();

  void OnInitialize(const bool success);

  void ShutdownBatAds();
  void OnShutdownBatAds(const bool success);

  bool StartService();

  void MaybeStart(const bool should_restart);
  void Start(const uint32_t number_of_start);
  void Stop();

  void ResetState();
  void OnShutdownAndResetBatAds(const bool success);
  void OnResetAllState(const bool success);

  void DetectUncertainFuture(const uint32_t number_of_start);
  void OnDetectUncertainFuture(const uint32_t number_of_start,
                               const bool is_uncertain_future);

  void EnsureBaseDirectoryExists(const uint32_t number_of_start);
  void OnEnsureBaseDirectoryExists(const uint32_t number_of_start,
                                   const bool success);

  void SetEnvironment();

  void SetBuildChannel();

  void UpdateIsDebugFlag();
  bool IsDebug() const;

  void StartCheckIdleStateTimer();
  void CheckIdleState();
  void ProcessIdleState(const ui::IdleState idle_state, const int idle_time);
  int GetIdleTimeThreshold();

  bool ShouldShowCustomAdNotifications();

  void MaybeOpenNewTabWithAd();
  void OpenNewTabWithAd(const std::string& uuid);
  void OnOpenNewTabWithAd(const std::string& json);
  void RetryOpeningNewTabWithAd(const std::string& uuid);

  void OpenNewTabWithUrl(const std::string& url);

  void NotificationTimedOut(const std::string& uuid);

  void RegisterResourceComponentsForLocale(const std::string& locale);

  void OnURLRequestStarted(
      const GURL& final_url,
      const network::mojom::URLResponseHead& response_head);

  void OnURLRequestComplete(SimpleURLLoaderList::iterator url_loader_it,
                            ads::UrlRequestCallback callback,
                            const std::unique_ptr<std::string> response_body);

  void OnGetBraveWallet(ledger::type::BraveWalletPtr wallet);

  void OnGetInlineContentAd(OnGetInlineContentAdCallback callback,
                            const bool success,
                            const std::string& dimensions,
                            const std::string& json);

  void OnGetAdsHistory(OnGetAdsHistoryCallback callback,
                       const std::string& json);

  void OnGetAccountStatement(GetAccountStatementCallback callback,
                             const bool success,
                             const std::string& json);

  void OnGetAdDiagnostics(GetAdDiagnosticsCallback callback,
                          const bool success,
                          const std::string& json);

  void OnRemoveAllHistory(const bool success);

  void OnToggleAdThumbUp(OnToggleAdThumbUpCallback callback,
                         const std::string& json);
  void OnToggleAdThumbDown(const OnToggleAdThumbDownCallback callback,
                           const std::string& json);

  void OnToggleAdOptIn(OnToggleAdOptInCallback callback,
                       const std::string& category,
                       const int action);
  void OnToggleAdOptOut(OnToggleAdOptOutCallback callback,
                        const std::string& category,
                        const int action);

  void OnToggleSavedAd(OnToggleSavedAdCallback callback,
                       const std::string& json);

  void OnToggleFlaggedAd(OnToggleFlaggedAdCallback callback,
                         const std::string& json);

  void OnLoaded(const ads::LoadCallback& callback, const std::string& value);
  void OnSaved(const ads::ResultCallback& callback, const bool success);

  void OnRunDBTransaction(ads::RunDBTransactionCallback callback,
                          ads::mojom::DBCommandResponsePtr response);

  void MigratePrefs();
  bool MigratePrefs(const int source_version,
                    const int dest_version,
                    const bool is_dry_run = false);
  void MigratePrefsVersion1To2();
  void MigratePrefsVersion2To3();
  void MigratePrefsVersion3To4();
  void MigratePrefsVersion4To5();
  void MigratePrefsVersion5To6();
  void MigratePrefsVersion6To7();
  void MigratePrefsVersion7To8();
  void MigratePrefsVersion8To9();
  void MigratePrefsVersion9To10();
  void MigratePrefsVersion10To11();

  bool IsUpgradingFromPreBraveAdsBuild();

  void DisableAdsIfUpgradingFromPreBraveAdsBuild();
  void DisableAdsForUnsupportedCountryCodes(
      const std::string& country_code,
      const std::vector<std::string>& country_codes);

  void MaybeShowMyFirstAdNotification();
  bool ShouldShowMyFirstAdNotification();

  bool PrefExists(const std::string& path) const;
  void OnPrefsChanged(const std::string& pref);

  std::string GetLocale() const;

  std::string LoadDataResourceAndDecompressIfNeeded(const int id) const;

  void StartNotificationTimeoutTimer(const std::string& uuid);
  bool StopNotificationTimeoutTimer(const std::string& uuid);

  bool connected();

  bool IsBraveNewsEnabled() const;
  bool ShouldStart() const;

  // AdsClient implementation
  bool IsNetworkConnectionAvailable() const override;

  bool IsForeground() const override;

  bool IsFullScreen() const override;

  bool ShouldShowNotifications() override;

  bool CanShowBackgroundNotifications() const override;

  void ShowNotification(const ads::AdNotificationInfo& info) override;

  void CloseNotification(const std::string& uuid) override;

  void RecordAdEvent(const std::string& type,
                     const std::string& confirmation_type,
                     const double timestamp) const override;

  std::vector<double> GetAdEvents(
      const std::string& ad_type,
      const std::string& confirmation_type) const override;

  void ResetAdEvents() const override;

  void UrlRequest(ads::mojom::UrlRequestPtr url_request,
                  ads::UrlRequestCallback callback) override;

  void Save(const std::string& name,
            const std::string& value,
            ads::ResultCallback callback) override;

  void Load(const std::string& name, ads::LoadCallback callback) override;

  void LoadAdsResource(const std::string& id,
                       const int version,
                       ads::LoadCallback callback) override;

  void GetBrowsingHistory(const int max_count,
                          const int days_ago,
                          ads::GetBrowsingHistoryCallback callback) override;

  void OnBrowsingHistorySearchComplete(ads::GetBrowsingHistoryCallback callback,
                                       history::QueryResults results);

  std::string LoadResourceForId(const std::string& id) override;

  void ClearScheduledCaptcha() override;

  void GetScheduledCaptcha(const std::string& payment_id,
                           ads::GetScheduledCaptchaCallback callback) override;

  void ShowScheduledCaptchaNotification(const std::string& payment_id,
                                        const std::string& captcha_id) override;

  void RunDBTransaction(ads::mojom::DBTransactionPtr transaction,
                        ads::RunDBTransactionCallback callback) override;

  void OnAdRewardsChanged() override;

  void RecordP2AEvent(const std::string& name,
                      const ads::mojom::P2AEventType type,
                      const std::string& value) override;

  void WriteDiagnosticLog(const std::string& file,
                          const int line,
                          const int verbose_level,
                          const std::string& message);

  void Log(const char* file,
           const int line,
           const int verbose_level,
           const std::string& message) override;

  bool GetBooleanPref(const std::string& path) const override;

  void SetBooleanPref(const std::string& path, const bool value) override;

  int GetIntegerPref(const std::string& path) const override;

  void SetIntegerPref(const std::string& path, const int value) override;

  double GetDoublePref(const std::string& path) const override;

  void SetDoublePref(const std::string& path, const double value) override;

  std::string GetStringPref(const std::string& path) const override;

  void SetStringPref(const std::string& path,
                     const std::string& value) override;

  int64_t GetInt64Pref(const std::string& path) const override;

  void SetInt64Pref(const std::string& path, const int64_t value) override;

  uint64_t GetUint64Pref(const std::string& path) const override;

  void SetUint64Pref(const std::string& path, const uint64_t value) override;

  void ClearPref(const std::string& path) override;

  // BackgroundHelper::Observer implementation
  void OnBackground() override;
  void OnForeground() override;

  raw_ptr<Profile> profile_ = nullptr;  // NOT OWNED

  raw_ptr<history::HistoryService> history_service_ = nullptr;  // NOT OWNED

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  brave_adaptive_captcha::BraveAdaptiveCaptchaService*
      adaptive_captcha_service_;  // NOT OWNED
  std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate_;
#endif

  bool is_initialized_ = false;

  bool deprecated_data_files_removed_ = false;

  bool is_upgrading_from_pre_brave_ads_build_;

  // This is needed to check if current ads service init become stale as
  // another ads service start is in progress
  uint32_t total_number_of_starts_ = 0;

  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  const base::FilePath base_path_;

  std::map<std::string, std::unique_ptr<base::OneShotTimer>>
      notification_timers_;

  std::string retry_opening_new_tab_for_ad_with_uuid_;

  base::OneShotTimer onboarding_timer_;

  std::unique_ptr<ads::Database> database_;

  ui::IdleState last_idle_state_;
  int last_idle_time_;

  base::RepeatingTimer idle_poll_timer_;

  PrefChangeRegistrar profile_pref_change_registrar_;

  SimpleURLLoaderList url_loaders_;

  raw_ptr<NotificationDisplayService> display_service_ = nullptr;  // NOT OWNED
  raw_ptr<brave_rewards::RewardsService> rewards_service_{
      nullptr};  // NOT OWNED

  mojo::AssociatedReceiver<bat_ads::mojom::BatAdsClient>
      bat_ads_client_receiver_;
  mojo::AssociatedRemote<bat_ads::mojom::BatAds> bat_ads_;
  mojo::Remote<bat_ads::mojom::BatAdsService> bat_ads_service_;

  // The task tracker for the HistoryService callbacks.
  base::CancelableTaskTracker task_tracker_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_IMPL_H_
