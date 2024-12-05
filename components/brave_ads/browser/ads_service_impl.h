/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/threading/sequence_bound.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "brave/components/brave_ads/browser/application_state/background_helper.h"
#include "brave/components/brave_ads/browser/component_updater/resource_component_observer.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/service/ads_service_callback.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom-forward.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/base/idle/idle.h"

class GURL;
class PrefService;

namespace base {
class OneShotTimer;
class SequencedTaskRunner;
}  // namespace base

namespace brave_rewards {
class RewardsService;
}  // namespace brave_rewards

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_ads {

class AdsServiceObserver;
class AdsTooltipsDelegate;
class BatAdsServiceFactory;
class Database;
class DeviceId;
class ResourceComponent;
struct NewTabPageAdInfo;

class AdsServiceImpl final : public AdsService,
                             public bat_ads::mojom::BatAdsClient,
                             public bat_ads::mojom::BatAdsObserver,
                             BackgroundHelper::Observer,
                             public ResourceComponentObserver,
                             public brave_rewards::RewardsServiceObserver {
 public:
  explicit AdsServiceImpl(
      std::unique_ptr<Delegate> delegate,
      PrefService* prefs,
      PrefService* local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader,
      std::string_view channel_name,
      const base::FilePath& profile_path,
      std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate,
      std::unique_ptr<DeviceId> device_id,
      std::unique_ptr<BatAdsServiceFactory> bat_ads_service_factory,
      brave_ads::ResourceComponent* resource_component,
      history::HistoryService* history_service,
      brave_rewards::RewardsService* rewards_service);

  AdsServiceImpl(const AdsServiceImpl&) = delete;
  AdsServiceImpl& operator=(const AdsServiceImpl&) = delete;

  ~AdsServiceImpl() override;

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  bool IsBatAdsServiceBound() const;

  void RegisterResourceComponents();
  void RegisterResourceComponentsForCurrentCountryCode();
  void RegisterResourceComponentsForDefaultLanguageCode();

  void Migrate();

  bool UserHasJoinedBraveRewards() const;
  bool UserHasOptedInToBraveNewsAds() const;
  bool UserHasOptedInToNewTabPageAds() const;
  bool UserHasOptedInToNotificationAds() const;
  bool UserHasOptedInToSearchResultAds() const;

  void InitializeNotificationsForCurrentProfile();

  void GetDeviceIdAndMaybeStartBatAdsService();
  void GetDeviceIdAndMaybeStartBatAdsServiceCallback(std::string device_id);

  bool CanStartBatAdsService() const;
  void MaybeStartBatAdsService();
  void StartBatAdsService();
  void DisconnectHandler();
  bool ShouldProceedInitialization(size_t current_start_number) const;
  void BatAdsServiceCreatedCallback(size_t current_start_number);
  void InitializeBasePathDirectoryCallback(size_t current_start_number,
                                           bool success);
  void Initialize(size_t current_start_number);
  void InitializeDatabase();
  void InitializeRewardsWallet(size_t current_start_number);
  void InitializeRewardsWalletCallback(
      size_t current_start_number,
      brave_rewards::mojom::RewardsWalletPtr mojom_rewards_wallet);
  void InitializeBatAds(
      brave_rewards::mojom::RewardsWalletPtr mojom_rewards_wallet);
  void InitializeBatAdsCallback(bool success);

  void NotifyAdsServiceInitialized() const;

  void ShutdownAndClearAdsServiceDataAndMaybeRestart(
      ClearDataCallback callback);
  void ShutdownAndClearPrefsAndAdsServiceDataAndMaybeRestart(
      ClearDataCallback callback);
  void ClearAdsServiceDataAndMaybeRestart(ClearDataCallback callback);
  void ClearAdsServiceDataAndMaybeRestartCallback(ClearDataCallback callback,
                                                  bool success);

  void OnExternalWalletConnectedCallback(bool success);

  void SetSysInfo();
  void SetBuildChannel();
  void SetFlags();

  bool ShouldShowOnboardingNotification();
  void MaybeShowOnboardingNotification();

  void ShowReminder(mojom::ReminderType mojom_reminder_type);

  void CloseAdaptiveCaptcha();

  void InitializeLocalStatePrefChangeRegistrar();
  void InitializePrefChangeRegistrar();
  void InitializeBraveRewardsPrefChangeRegistrar();
  void InitializeSubdivisionTargetingPrefChangeRegistrar();
  void InitializeBraveNewsAdsPrefChangeRegistrar();
  void InitializeNewTabPageAdsPrefChangeRegistrar();
  void InitializeNotificationAdsPrefChangeRegistrar();
  void InitializeSearchResultAdsPrefChangeRegistrar();
  void OnOptedInToAdsPrefChanged(const std::string& path);
  void OnCountryCodePrefChanged(const std::string& path);
  void NotifyPrefChanged(const std::string& path) const;

  void GetRewardsWallet();
  void NotifyRewardsWalletDidUpdate(
      brave_rewards::mojom::RewardsWalletPtr mojom_rewards_wallet);

  // TODO(https://github.com/brave/brave-browser/issues/14666) Decouple idle
  // state business logic.
  void CheckIdleStateAfterDelay();
  void CheckIdleState();
  void ProcessIdleState(ui::IdleState idle_state, base::TimeDelta idle_time);

  // TODO(https://github.com/brave/brave-browser/issues/23974) Decouple
  // notification ad business logic.
  bool CheckIfCanShowNotificationAds();
  bool ShouldShowCustomNotificationAds();
  void StartNotificationAdTimeOutTimer(const std::string& placement_id);
  bool StopNotificationAdTimeOutTimer(const std::string& placement_id);
  void NotificationAdTimedOut(const std::string& placement_id);
  void CloseAllNotificationAds();

  // TODO(https://github.com/brave/brave-browser/issues/26192) Decouple new
  // tab page ad business logic.
  void PrefetchNewTabPageAdCallback(std::optional<base::Value::Dict> dict);

  // TODO(https://github.com/brave/brave-browser/issues/26193) Decouple open
  // new tab with ad business logic.
  void MaybeOpenNewTabWithAd();
  void OpenNewTabWithAd(const std::string& placement_id);
  void OpenNewTabWithAdCallback(std::optional<base::Value::Dict> dict);
  void RetryOpeningNewTabWithAd(const std::string& placement_id);
  void OpenNewTabWithUrl(const GURL& url);

  // TODO(https://github.com/brave/brave-browser/issues/14676) Decouple URL
  // request business logic.
  void URLRequestCallback(SimpleURLLoaderList::iterator url_loader_iter,
                          UrlRequestCallback callback,
                          std::unique_ptr<std::string> response_body);

  void ShowScheduledCaptchaCallback(const std::string& payment_id,
                                    const std::string& captcha_id);
  void SnoozeScheduledCaptchaCallback();

  void OnNotificationAdPositionChanged();

  void ShutdownAdsService();

  // KeyedService:
  void Shutdown() override;

  // AdsService:
  void AddBatAdsObserver(mojo::PendingRemote<bat_ads::mojom::BatAdsObserver>
                             bat_ads_observer_pending_remote) override;

  bool IsBrowserUpgradeRequiredToServeAds() const override;

  int64_t GetMaximumNotificationAdsPerHour() const override;

  void OnNotificationAdShown(const std::string& placement_id) override;
  void OnNotificationAdClosed(const std::string& placement_id,
                              bool by_user) override;
  void OnNotificationAdClicked(const std::string& placement_id) override;

  void ClearData(ClearDataCallback callback) override;

  void GetInternals(GetInternalsCallback callback) override;

  void GetDiagnostics(GetDiagnosticsCallback callback) override;

  void GetStatementOfAccounts(GetStatementOfAccountsCallback callback) override;

  void MaybeServeInlineContentAd(
      const std::string& dimensions,
      MaybeServeInlineContentAdCallback callback) override;
  void TriggerInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  std::optional<NewTabPageAdInfo> MaybeGetPrefetchedNewTabPageAdForDisplay()
      override;
  void PrefetchNewTabPageAd() override;
  void OnFailedToPrefetchNewTabPageAd(
      const std::string& placement_id,
      const std::string& creative_instance_id) override;
  void TriggerNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  void TriggerPromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  void MaybeGetSearchResultAd(const std::string& placement_id,
                              MaybeGetSearchResultAdCallback callback) override;
  void TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad,
      mojom::SearchResultAdEventType mojom_ad_event_type,
      TriggerAdEventCallback callback) override;

  void PurgeOrphanedAdEventsForType(
      mojom::AdType mojom_ad_type,
      PurgeOrphanedAdEventsForTypeCallback callback) override;

  void GetAdHistory(base::Time from_time,
                    base::Time to_time,
                    GetAdHistoryForUICallback callback) override;

  void ToggleLikeAd(mojom::ReactionInfoPtr mojom_reaction,
                    ToggleReactionCallback callback) override;
  void ToggleDislikeAd(mojom::ReactionInfoPtr mojom_reaction,
                       ToggleReactionCallback callback) override;
  void ToggleLikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                         ToggleReactionCallback callback) override;
  void ToggleDislikeSegment(mojom::ReactionInfoPtr mojom_reaction,
                            ToggleReactionCallback callback) override;
  void ToggleSaveAd(mojom::ReactionInfoPtr mojom_reaction,
                    ToggleReactionCallback callback) override;
  void ToggleMarkAdAsInappropriate(mojom::ReactionInfoPtr mojom_reaction,
                                   ToggleReactionCallback callback) override;

  void NotifyTabTextContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& text) override;
  void NotifyTabHtmlContentDidChange(int32_t tab_id,
                                     const std::vector<GURL>& redirect_chain,
                                     const std::string& html) override;
  void NotifyTabDidStartPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidStopPlayingMedia(int32_t tab_id) override;
  void NotifyTabDidChange(int32_t tab_id,
                          const std::vector<GURL>& redirect_chain,
                          bool is_new_navigation,
                          bool is_restoring,
                          bool is_visible) override;
  void NotifyTabDidLoad(int32_t tab_id, int http_status_code) override;
  void NotifyDidCloseTab(int32_t tab_id) override;

  void NotifyUserGestureEventTriggered(int32_t page_transition_type) override;

  void NotifyBrowserDidBecomeActive() override;
  void NotifyBrowserDidResignActive() override;

  void NotifyDidSolveAdaptiveCaptcha() override;

  // bat_ads::mojom::BatAdsClient:
  void IsNetworkConnectionAvailable(
      IsNetworkConnectionAvailableCallback callback) override;

  void IsBrowserActive(IsBrowserActiveCallback callback) override;
  void IsBrowserInFullScreenMode(
      IsBrowserInFullScreenModeCallback callback) override;

  void CanShowNotificationAds(CanShowNotificationAdsCallback callback) override;
  void CanShowNotificationAdsWhileBrowserIsBackgrounded(
      CanShowNotificationAdsWhileBrowserIsBackgroundedCallback callback)
      override;
  void ShowNotificationAd(base::Value::Dict dict) override;
  void CloseNotificationAd(const std::string& placement_id) override;

  void GetSiteHistory(int max_count,
                      int recent_day_range,
                      GetSiteHistoryCallback callback) override;

  // TODO(https://github.com/brave/brave-browser/issues/14676) Decouple URL
  // request business logic.
  void UrlRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                  UrlRequestCallback callback) override;

  // TODO(https://github.com/brave/brave-browser/issues/26194) Decouple
  // load/save file business logic.
  void Save(const std::string& name,
            const std::string& value,
            SaveCallback callback) override;
  void Load(const std::string& name, LoadCallback callback) override;

  // TODO(https://github.com/brave/brave-browser/issues/26195) Decouple load
  // resources business logic.
  void LoadResourceComponent(const std::string& id,
                             int version,
                             LoadResourceComponentCallback callback) override;
  void LoadDataResource(const std::string& name,
                        LoadDataResourceCallback callback) override;

  void ShowScheduledCaptcha(const std::string& payment_id,
                            const std::string& captcha_id) override;

  void RunDBTransaction(mojom::DBTransactionInfoPtr mojom_db_transaction,
                        RunDBTransactionCallback callback) override;

  // TODO(https://github.com/brave/brave-browser/issues/14666) Decouple P2A
  // business logic.
  void RecordP2AEvents(const std::vector<std::string>& events) override;

  void FindProfilePref(const std::string& path,
                       FindProfilePrefCallback callback) override;
  void GetProfilePref(const std::string& path,
                      GetProfilePrefCallback callback) override;
  void SetProfilePref(const std::string& path, base::Value value) override;
  void ClearProfilePref(const std::string& path) override;
  void HasProfilePrefPath(const std::string& path,
                          HasProfilePrefPathCallback callback) override;

  void FindLocalStatePref(const std::string& path,
                          FindLocalStatePrefCallback callback) override;
  void GetLocalStatePref(const std::string& path,
                         GetLocalStatePrefCallback callback) override;
  void SetLocalStatePref(const std::string& path, base::Value value) override;
  void ClearLocalStatePref(const std::string& path) override;
  void HasLocalStatePrefPath(const std::string& path,
                             HasLocalStatePrefPathCallback callback) override;

  void GetVirtualPrefs(GetVirtualPrefsCallback callback) override;

  void Log(const std::string& file,
           int32_t line,
           int32_t verbose_level,
           const std::string& message) override;

  // bat_ads::mojom::BatAdsObserver:
  void OnAdRewardsDidChange() override {}
  void OnBrowserUpgradeRequiredToServeAds() override;
  void OnIneligibleWalletToServeAds() override {}
  void OnRemindUser(mojom::ReminderType mojom_reminder_type) override;

  // BackgroundHelper::Observer:
  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  // ResourceComponentObserver:
  void OnResourceComponentDidChange(const std::string& manifest_version,
                                    const std::string& id) override;
  void OnDidUnregisterResourceComponent(const std::string& id) override;

  // RewardsServiceObserver:
  void OnRewardsWalletCreated() override;
  void OnExternalWalletConnected() override;
  void OnCompleteReset(bool success) override;

  bool is_bat_ads_initialized_ = false;

  bool is_shutting_down_ = false;

  bool browser_upgrade_required_to_serve_ads_ = false;

  // Brave Ads Service starts count is needed to avoid possible double Brave
  // Ads initialization.
  // TODO(https://github.com/brave/brave-browser/issues/30247): Refactor Brave
  // Ads startup logic.
  size_t service_starts_count_ = 0;

  PrefChangeRegistrar pref_change_registrar_;

  PrefChangeRegistrar local_state_pref_change_registrar_;

  mojom::SysInfo sys_info_;

  base::SequenceBound<Database> database_;

  base::RepeatingTimer idle_state_timer_;
  ui::IdleState last_idle_state_ = ui::IdleState::IDLE_STATE_ACTIVE;
  base::TimeDelta last_idle_time_;

  std::map<std::string, std::unique_ptr<base::OneShotTimer>>
      notification_ad_timers_;

  std::optional<NewTabPageAdInfo> prefetched_new_tab_page_ad_;
  bool is_prefetching_new_tab_page_ad_ = false;

  std::string retry_opening_new_tab_for_ad_with_placement_id_;

  base::CancelableTaskTracker history_service_task_tracker_;

  SimpleURLLoaderList url_loaders_;

  const raw_ptr<PrefService> prefs_ = nullptr;  // Not owned.

  const raw_ptr<PrefService> local_state_ = nullptr;  // Not owned.

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_ = nullptr;

  const std::string channel_name_;

  base::ScopedObservation<brave_ads::ResourceComponent,
                          ResourceComponentObserver>
      resource_component_observation_{this};

  const raw_ptr<history::HistoryService> history_service_ =
      nullptr;  // Not owned.

  const std::unique_ptr<AdsTooltipsDelegate> ads_tooltips_delegate_;

  const std::unique_ptr<DeviceId> device_id_;

  const std::unique_ptr<BatAdsServiceFactory> bat_ads_service_factory_;

  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  const base::FilePath ads_service_path_;

  base::ScopedObservation<brave_rewards::RewardsService,
                          brave_rewards::RewardsServiceObserver>
      rewards_service_observation_{this};

  base::ObserverList<AdsServiceObserver> observers_;

  mojo::Receiver<bat_ads::mojom::BatAdsObserver> bat_ads_observer_receiver_{
      this};
  mojo::Remote<bat_ads::mojom::BatAdsService> bat_ads_service_remote_;
  mojo::AssociatedReceiver<bat_ads::mojom::BatAdsClient>
      bat_ads_client_associated_receiver_;
  mojo::AssociatedRemote<bat_ads::mojom::BatAds> bat_ads_associated_remote_;
  mojo::Remote<bat_ads::mojom::BatAdsClientNotifier>
      bat_ads_client_notifier_remote_;
  mojo::PendingReceiver<bat_ads::mojom::BatAdsClientNotifier>
      bat_ads_client_notifier_pending_receiver_;

  base::WeakPtrFactory<AdsServiceImpl> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_IMPL_H_
