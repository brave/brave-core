/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_

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
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/browser/brave_ads/background_helper/background_helper.h"
#include "brave/components/brave_adaptive_captcha/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/base/idle/idle.h"

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#endif

class GURL;

class NotificationDisplayService;
class Profile;

namespace ads {
class Database;
struct NewTabPageAdInfo;
struct NotificationAdInfo;
}  // namespace ads

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_federated {
class AsyncDataStore;
}  // namespace brave_federated

namespace brave_rewards {
class RewardsService;
}  // namespace brave_rewards

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace brave_ads {

class AdsTooltipsDelegate;
class DeviceId;

class AdsServiceImpl : public AdsService,
                       public ads::AdsClient,
                       BackgroundHelper::Observer,
                       public Observer,
                       public base::SupportsWeakPtr<AdsServiceImpl> {
 public:
  explicit AdsServiceImpl(
      Profile* profile,
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
      brave_adaptive_captcha::BraveAdaptiveCaptchaService*
          adaptive_captcha_service,
      std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate,
#endif
      std::unique_ptr<DeviceId> device_id,
      history::HistoryService* history_service,
      brave_rewards::RewardsService* rewards_service,
      brave_federated::AsyncDataStore* notification_ad_timing_data_store);
  AdsServiceImpl(const AdsServiceImpl&) = delete;
  AdsServiceImpl& operator=(const AdsServiceImpl&) = delete;
  ~AdsServiceImpl() override;

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  bool IsBraveNewsEnabled() const;

  bool ShouldStartBatAds() const;

  void InitNotificationsForProfile();

  void MigrateConfirmationState();
  void OnMigrateConfirmationState(const bool success);

  void InitializePrefChangeRegistrar();

  void SetSysInfo();

  void SetBuildChannel();

  void MaybeStartOrStop(const bool should_restart);
  void StartBatAdsService();
  base::TimeDelta GetBatAdsServiceRestartDelay();
  void ShutdownBatAds();
  void OnShutdownBatAds(const bool success);
  void Start(const uint32_t number_of_start);

  void GetDeviceId(const uint32_t number_of_start);
  void OnGetDeviceId(const uint32_t number_of_start, std::string device_id);

  void DetectUncertainFuture(const uint32_t number_of_start);
  void OnDetectUncertainFuture(const uint32_t number_of_start,
                               const bool is_uncertain_future);

  void EnsureBaseDirectoryExists(const uint32_t number_of_start);
  void OnEnsureBaseDirectoryExists(const uint32_t number_of_start,
                                   const bool success);

  void CreateBatAdsService(const uint32_t number_of_start);
  void OnCreateBatAdsService();

  bool IsBatAdsServiceBound() const;
  bool IsBatAdsBound() const;

  void OnInitializeBatAds(const bool success);

  void CleanUpOnFirstRun();
  void RemoveDeprecatedFiles() const;

  void ResetState();
  void OnResetState(const bool success);
  void OnShutdownAndResetState(const bool success);

  void OnEnabledPrefChanged();
  void OnIdleTimeThresholdPrefChanged();
  void OnWalletBravePrefChanged();
  void OnBraveTodayOptedInPrefChanged();
  void OnNewTabPageShowTodayPrefChanged();

  void NotifyPrefChanged(const std::string& path);

  bool ShouldShowOnboardingNotification();
  void MaybeShowOnboardingNotification();

  void GetRewardsWallet();
  void OnGetRewardsWallet(ledger::type::BraveWalletPtr wallet);

  void StartCheckIdleStateTimer();
  void CheckIdleState();
  void ProcessIdleState(const ui::IdleState idle_state,
                        const base::TimeDelta idle_time);

  void OnMaybeServeInlineContentAd(MaybeServeInlineContentAdCallback callback,
                                   const std::string& dimensions,
                                   absl::optional<base::Value::Dict> dict);

  absl::optional<ads::NewTabPageAdInfo> GetPrefetchedNewTabPageAd() override;
  void OnFailedToPrefetchNewTabPageAd(
      const std::string& placement_id,
      const std::string& creative_instance_id) override;

  bool ShouldShowCustomNotificationAds();
  void StartNotificationAdTimeOutTimer(const std::string& placement_id);
  bool StopNotificationAdTimeOutTimer(const std::string& placement_id);
  void NotificationAdTimedOut(const std::string& placement_id);

  void PrefetchNewTabPageAd() override;
  void OnPrefetchNewTabPageAd(absl::optional<base::Value::Dict> dict);

  void OnPurgeOrphanedNewTabPageAdEvents(const bool success);

  void OpenNewTabWithUrl(const GURL& url);
  void MaybeOpenNewTabWithAd();
  void OpenNewTabWithAd(const std::string& placement_id);
  void OnGetNotificationAd(absl::optional<base::Value::Dict> dict);
  void RetryOpeningNewTabWithAd(const std::string& placement_id);

  bool IsUpgradingFromPreBraveAdsBuild();
  void MigratePrefs();
  bool MigratePrefs(const int source_version,
                    const int dest_version,
                    const bool is_dry_run = false);
  void DisableAdsIfUpgradingFromPreBraveAdsBuild();
  void DisableAdsForUnsupportedCountryCodes(
      const std::string& country_code,
      const std::vector<std::string>& country_codes);
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
  void MigratePrefsVersion11To12();

  void WriteDiagnosticLog(const std::string& file,
                          const int line,
                          const int verbose_level,
                          const std::string& message);

  void OnGetDiagnostics(GetDiagnosticsCallback callback,
                        absl::optional<base::Value::List> value);

  void OnGetStatementOfAccounts(GetStatementOfAccountsCallback callback,
                                ads::mojom::StatementInfoPtr statement);

  void OnGetHistory(GetHistoryCallback callback, const std::string& json);

  void OnToggleAdThumbUp(ToggleAdThumbUpCallback callback,
                         const std::string& json);
  void OnToggleAdThumbDown(const ToggleAdThumbDownCallback callback,
                           const std::string& json);
  void OnToggleAdOptIn(ToggleAdOptInCallback callback,
                       const std::string& category,
                       const int action);
  void OnToggleAdOptOut(ToggleAdOptOutCallback callback,
                        const std::string& category,
                        const int action);
  void OnToggleSavedAd(ToggleSavedAdCallback callback, const std::string& json);
  void OnToggleFlaggedAd(ToggleFlaggedAdCallback callback,
                         const std::string& json);

  void OnBrowsingHistorySearchComplete(ads::GetBrowsingHistoryCallback callback,
                                       history::QueryResults results);

  void OnURLRequest(SimpleURLLoaderList::iterator url_loader_iter,
                    ads::UrlRequestCallback callback,
                    const std::unique_ptr<std::string> response_body);

  void OnSave(ads::SaveCallback callback, const bool success);
  void OnLoad(ads::LoadCallback callback, const std::string& value);
  void OnLoadFileResource(
      ads::LoadFileCallback callback,
      std::unique_ptr<base::File, base::OnTaskRunnerDeleter> file);

  void OnRunDBTransaction(ads::RunDBTransactionCallback callback,
                          ads::mojom::DBCommandResponseInfoPtr response);

  void OnLogTrainingInstance(bool success);

  // KeyedService:
  void Shutdown() override;

  // AdsService:
  bool IsSupportedLocale() const override;

  bool IsEnabled() const override;
  void SetEnabled(const bool is_enabled) override;

  int64_t GetMaximumNotificationAdsPerHour() const override;
  void SetMaximumNotificationAdsPerHour(const int64_t ads_per_hour) override;

  void SetAllowConversionTracking(const bool should_allow) override;

  bool ShouldAllowSubdivisionTargeting() const override;
  std::string GetSubdivisionTargetingCode() const override;
  void SetSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) override;
  std::string GetAutoDetectedSubdivisionTargetingCode() const override;
  void SetAutoDetectedSubdivisionTargetingCode(
      const std::string& subdivision_targeting_code) override;

  bool NeedsBrowserUpgradeToServeAds() const override;

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  void ShowScheduledCaptcha(const std::string& payment_id,
                            const std::string& captcha_id) override;
  void SnoozeScheduledCaptcha() override;
#endif

  void OnNotificationAdShown(const std::string& placement_id) override;
  void OnNotificationAdClosed(const std::string& placement_id,
                              const bool by_user) override;
  void OnNotificationAdClicked(const std::string& placement_id) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void OnChangeLocale(const std::string& locale) override;

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
                    const std::vector<GURL>& redirect_chain,
                    const bool is_active,
                    const bool is_browser_active) override;
  void OnTabClosed(const SessionID& tab_id) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::InlineContentAdEventType event_type) override;

  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::NewTabPageAdEventType event_type) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const ads::mojom::PromotedContentAdEventType event_type) override;

  void TriggerSearchResultAdEvent(
      ads::mojom::SearchResultAdInfoPtr ad_mojom,
      const ads::mojom::SearchResultAdEventType event_type) override;

  void PurgeOrphanedAdEventsForType(
      const ads::mojom::AdType ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) override;

  void GetHistory(const base::Time from_time,
                  const base::Time to_time,
                  GetHistoryCallback callback) override;

  void ToggleAdThumbUp(const std::string& json,
                       ToggleAdThumbUpCallback callback) override;
  void ToggleAdThumbDown(const std::string& json,
                         ToggleAdThumbDownCallback callback) override;
  void ToggleAdOptIn(const std::string& category,
                     const int action,
                     const ToggleAdOptInCallback callback) override;
  void ToggleAdOptOut(const std::string& category,
                      const int action,
                      ToggleAdOptOutCallback callback) override;
  void ToggleSavedAd(const std::string& json,
                     ToggleSavedAdCallback callback) override;
  void ToggleFlaggedAd(const std::string& json,
                       ToggleFlaggedAdCallback callback) override;

  void WipeState(const bool should_shutdown) override;

  // AdsClient:
  bool IsNetworkConnectionAvailable() const override;

  bool IsBrowserActive() const override;
  bool IsBrowserInFullScreenMode() const override;

  bool CanShowNotificationAds() override;
  bool CanShowNotificationAdsWhileBrowserIsBackgrounded() const override;
  void ShowNotificationAd(const ads::NotificationAdInfo& ad) override;
  void CloseNotificationAd(const std::string& placement_id) override;

  void UpdateAdRewards() override;

  void RecordAdEventForId(const std::string& id,
                          const std::string& type,
                          const std::string& confirmation_type,
                          const base::Time time) const override;
  std::vector<base::Time> GetAdEventHistory(
      const std::string& ad_type,
      const std::string& confirmation_type) const override;
  void ResetAdEventHistoryForId(const std::string& id) const override;

  void GetBrowsingHistory(const int max_count,
                          const int days_ago,
                          ads::GetBrowsingHistoryCallback callback) override;

  void UrlRequest(ads::mojom::UrlRequestInfoPtr url_request,
                  ads::UrlRequestCallback callback) override;

  void Save(const std::string& name,
            const std::string& value,
            ads::SaveCallback callback) override;
  void Load(const std::string& name, ads::LoadCallback callback) override;
  void LoadFileResource(const std::string& id,
                        const int version,
                        ads::LoadFileCallback callback) override;
  std::string LoadDataResource(const std::string& name) override;

  void GetScheduledCaptcha(const std::string& payment_id,
                           ads::GetScheduledCaptchaCallback callback) override;
  void ShowScheduledCaptchaNotification(const std::string& payment_id,
                                        const std::string& captcha_id) override;
  void ClearScheduledCaptcha() override;

  void RunDBTransaction(ads::mojom::DBTransactionInfoPtr transaction,
                        ads::RunDBTransactionCallback callback) override;

  void RecordP2AEvent(const std::string& name,
                      base::Value::List value) override;

  void LogTrainingInstance(std::vector<brave_federated::mojom::CovariateInfoPtr>
                               training_instance) override;

  bool GetBooleanPref(const std::string& path) const override;
  int GetIntegerPref(const std::string& path) const override;
  double GetDoublePref(const std::string& path) const override;
  std::string GetStringPref(const std::string& path) const override;
  int64_t GetInt64Pref(const std::string& path) const override;
  uint64_t GetUint64Pref(const std::string& path) const override;
  base::Time GetTimePref(const std::string& path) const override;
  absl::optional<base::Value::Dict> GetDictPref(
      const std::string& path) const override;
  absl::optional<base::Value::List> GetListPref(
      const std::string& path) const override;

  void SetBooleanPref(const std::string& path, const bool value) override;
  void SetIntegerPref(const std::string& path, const int value) override;
  void SetDoublePref(const std::string& path, const double value) override;
  void SetStringPref(const std::string& path,
                     const std::string& value) override;
  void SetInt64Pref(const std::string& path, const int64_t value) override;
  void SetUint64Pref(const std::string& path, const uint64_t value) override;
  void SetTimePref(const std::string& path, const base::Time value) override;
  void SetDictPref(const std::string& path, base::Value::Dict value) override;
  void SetListPref(const std::string& path, base::Value::List value) override;

  void ClearPref(const std::string& path) override;

  bool HasPrefPath(const std::string& path) const override;

  void Log(const char* file,
           const int line,
           const int verbose_level,
           const std::string& message) override;

  // BackgroundHelper::Observer:
  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  // Observer:
  void OnResourceComponentUpdated(const std::string& id) override;

  bool is_bat_ads_initialized_ = false;
  bool did_cleanup_on_first_run_ = false;
  bool needs_browser_upgrade_to_serve_ads_ = false;
  bool is_upgrading_from_pre_brave_ads_build_ = false;

  // This is needed to check if the current ads service becomes stale due to
  // another ads service being in progress.
  uint32_t total_number_of_starts_ = 0;

  base::Time last_bat_ads_service_restart_time_;

  PrefChangeRegistrar pref_change_registrar_;

  ads::mojom::SysInfo sys_info_;

  std::unique_ptr<ads::Database> database_;

  base::RepeatingTimer idle_state_timer_;
  ui::IdleState last_idle_state_ = ui::IdleState::IDLE_STATE_ACTIVE;
  base::TimeDelta last_idle_time_;

  std::map<std::string, std::unique_ptr<base::OneShotTimer>>
      notification_ad_timers_;

  absl::optional<ads::NewTabPageAdInfo> prefetched_new_tab_page_ad_;
  bool need_purge_orphaned_new_tab_page_ad_events_ = false;
  bool prefetch_new_tab_page_ad_on_first_run_ = false;

  std::string retry_opening_new_tab_for_ad_with_placement_id_;

  base::CancelableTaskTracker history_service_task_tracker_;

  SimpleURLLoaderList url_loaders_;

  const raw_ptr<Profile> profile_ = nullptr;  // NOT OWNED

  const raw_ptr<history::HistoryService> history_service_ =
      nullptr;  // NOT OWNED

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  const raw_ptr<brave_adaptive_captcha::BraveAdaptiveCaptchaService>
      adaptive_captcha_service_ = nullptr;  // NOT OWNED
  const std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate_;
#endif

  const std::unique_ptr<DeviceId> device_id_;

  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  const base::FilePath base_path_;

  const raw_ptr<NotificationDisplayService> display_service_ =
      nullptr;  // NOT OWNED
  const raw_ptr<brave_rewards::RewardsService> rewards_service_{
      nullptr};  // NOT OWNED

  const raw_ptr<brave_federated::AsyncDataStore>
      notification_ad_timing_data_store_ = nullptr;  // NOT OWNED

  mojo::Remote<bat_ads::mojom::BatAdsService> bat_ads_service_;
  mojo::AssociatedReceiver<bat_ads::mojom::BatAdsClient> bat_ads_client_;
  mojo::AssociatedRemote<bat_ads::mojom::BatAds> bat_ads_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_
