/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "bat/ledger/ledger.h"
#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/one_shot_event.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/buildflags/buildflags.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "ui/gfx/image/image.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"
#include "brave/components/brave_rewards/browser/rewards_service_private_observer.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/components/brave_rewards/browser/extension_rewards_service_observer.h"
#endif

#if defined(OS_ANDROID) && defined(BRAVE_CHROMIUM_BUILD)
#include "brave/components/brave_rewards/browser/android/safetynet_check.h"
#elif defined(OS_ANDROID)
#include "chrome/browser/android/brave/safetynet_check.h"
#endif

namespace base {
class OneShotTimer;
class RepeatingTimer;
class SequencedTaskRunner;
}  // namespace base

namespace ledger {
class Ledger;
class LedgerCallbackHandler;
struct LedgerMediaPublisherInfo;
}  // namespace ledger

namespace leveldb {
class DB;
}  // namespace leveldb

namespace network {
class SimpleURLLoader;
}  // namespace network


class Profile;
class BraveRewardsBrowserTest;

namespace brave_rewards {

class PublisherInfoDatabase;
class RewardsNotificationServiceImpl;
class BraveRewardsBrowserTest;

using GetEnvironmentCallback = base::Callback<void(ledger::Environment)>;
using GetDebugCallback = base::Callback<void(bool)>;
using GetReconcileTimeCallback = base::Callback<void(int32_t)>;
using GetShortRetriesCallback = base::Callback<void(bool)>;
using GetTestResponseCallback =
    base::Callback<void(const std::string& url,
                        int32_t method,
                        int* response_status_code,
                        std::string* response,
                        std::map<std::string, std::string>* headers)>;

using ExternalWalletAuthorizationCallback =
    base::OnceCallback<void(
        const ledger::Result,
        const std::map<std::string, std::string>&)>;

class RewardsServiceImpl : public RewardsService,
                           public ledger::LedgerClient,
                           public base::SupportsWeakPtr<RewardsServiceImpl> {
 public:
  explicit RewardsServiceImpl(Profile* profile);
  ~RewardsServiceImpl() override;

  // KeyedService:
  void Shutdown() override;

  void Init();
  void StartLedger();
  void CreateWallet(CreateWalletCallback callback) override;
  void FetchWalletProperties() override;
  void FetchPromotions() override;
  void ClaimPromotion(ClaimPromotionCallback callback) override;
  void ClaimPromotion(
      const std::string& promotion_id,
      AttestPromotionCallback callback) override;
  void AttestPromotion(
      const std::string& promotion_id,
      const std::string& solution,
      AttestPromotionCallback callback) override;
  void GetWalletPassphrase(
      const GetWalletPassphraseCallback& callback) override;
  void RecoverWallet(const std::string& passPhrase) override;
  void GetContentSiteList(
      uint32_t start,
      uint32_t limit,
      uint64_t min_visit_time,
      uint64_t reconcile_stamp,
      bool allow_non_verified,
      uint32_t min_visits,
      bool fetch_excluded,
      const GetContentSiteListCallback& callback) override;
  void OnGetContentSiteList(
      const GetContentSiteListCallback& callback,
      ledger::PublisherInfoList list,
      uint32_t next_record);
  void OnLoad(SessionID tab_id, const GURL& url) override;
  void OnUnload(SessionID tab_id) override;
  void OnShow(SessionID tab_id) override;
  void OnHide(SessionID tab_id) override;
  void OnForeground(SessionID tab_id) override;
  void OnBackground(SessionID tab_id) override;
  void OnXHRLoad(SessionID tab_id,
                 const GURL& url,
                 const GURL& first_party_url,
                 const GURL& referrer) override;
  void OnPostData(SessionID tab_id,
                  const GURL& url,
                  const GURL& first_party_url,
                  const GURL& referrer,
                  const std::string& post_data) override;
  std::string URIEncode(const std::string& value) override;
  void GetReconcileStamp(const GetReconcileStampCallback& callback) override;
  void GetAutoContribute(
      GetAutoContributeCallback callback) override;
  void GetPublisherMinVisitTime(
      const GetPublisherMinVisitTimeCallback& callback) override;
  void GetPublisherMinVisits(
      const GetPublisherMinVisitsCallback& callback) override;
  void GetPublisherAllowNonVerified(
      const GetPublisherAllowNonVerifiedCallback& callback) override;
  void GetPublisherAllowVideos(
      const GetPublisherAllowVideosCallback& callback) override;
  void LoadPublisherInfo(
      const std::string& publisher_key,
      ledger::PublisherInfoCallback callback) override;
  void LoadMediaPublisherInfo(
      const std::string& media_key,
      ledger::PublisherInfoCallback callback) override;
  void SaveMediaPublisherInfo(const std::string& media_key,
                              const std::string& publisher_id) override;
  void RestorePublishersUI() override;
  void GetBalanceReport(
      const uint32_t month,
      const uint32_t year,
      GetBalanceReportCallback callback) override;
  void IsWalletCreated(const IsWalletCreatedCallback& callback) override;
  void GetPublisherActivityFromUrl(
      uint64_t window_id,
      const std::string& url,
      const std::string& favicon_url,
      const std::string& publisher_blob) override;
  void GetContributionAmount(
      const GetContributionAmountCallback& callback) override;
  void GetPublisherBanner(const std::string& publisher_id,
                          GetPublisherBannerCallback callback) override;
  void OnPublisherBanner(GetPublisherBannerCallback callback,
                         ledger::PublisherBannerPtr banner);
  void RemoveRecurringTipUI(const std::string& publisher_key) override;
  void OnGetRecurringTipsUI(
      GetRecurringTipsCallback callback,
      ledger::PublisherInfoList list);
  void GetRecurringTipsUI(GetRecurringTipsCallback callback) override;
  void GetOneTimeTips(
      ledger::PublisherInfoListCallback callback) override;
  void SetPublisherExclude(
      const std::string& publisher_key,
      bool exclude) override;
  RewardsNotificationService* GetNotificationService() const override;
  bool CheckImported() override;
  void SetBackupCompleted() override;
  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;

  void HandleFlags(const std::string& options);
  void SetEnvironment(ledger::Environment environment);
  void GetEnvironment(const GetEnvironmentCallback& callback);
  void SetDebug(bool debug);
  void GetDebug(const GetDebugCallback& callback);
  void SetReconcileTime(int32_t time);
  void GetReconcileTime(const GetReconcileTimeCallback& callback);
  void SetShortRetries(bool short_retries);
  void GetShortRetries(const GetShortRetriesCallback& callback);

  void GetAutoContributeProps(
      const GetAutoContributePropsCallback& callback) override;
  void GetPendingContributionsTotalUI(
      const GetPendingContributionsTotalCallback& callback) override;
  void GetRewardsMainEnabled(
      const GetRewardsMainEnabledCallback& callback) const override;

  void GetOneTimeTipsUI(GetOneTimeTipsCallback callback) override;
  void RefreshPublisher(
      const std::string& publisher_key,
      RefreshPublisherCallback callback) override;
  void OnAdsEnabled(bool ads_enabled) override;

  void OnSaveRecurringTipUI(
      SaveRecurringTipCallback callback,
      const ledger::Result result);
  void SaveRecurringTipUI(
      const std::string& publisher_key,
      const double amount,
      SaveRecurringTipCallback callback) override;

  const RewardsNotificationService::RewardsNotificationsMap&
    GetAllNotifications() override;
  void ResetTheWholeState(const base::Callback<void(bool)>& callback) override;

  void SetContributionAmount(const double amount) const override;

  void SaveInlineMediaInfo(
      const std::string& media_type,
      const std::map<std::string, std::string>& args,
      SaveMediaInfoCallback callback) override;

  void SetInlineTipSetting(const std::string& key, bool enabled) override;

  void GetInlineTipSetting(
      const std::string& key,
      GetInlineTipSettingCallback callback) override;

  void GetShareURL(
      const std::string& type,
      const std::map<std::string, std::string>& args,
      GetShareURLCallback callback) override;

  void GetPendingContributionsUI(
    GetPendingContributionsCallback callback) override;

  void RemovePendingContributionUI(const uint64_t id) override;

  void RemoveAllPendingContributionsUI() override;

  void OnTip(const std::string& publisher_key,
             double amount,
             bool recurring) override;

  void OnTip(
      const std::string& publisher_key,
      const double amount,
      const bool recurring,
      std::unique_ptr<brave_rewards::ContentSite> site) override;

  void SetPublisherMinVisitTime(uint64_t duration_in_seconds) const override;

  void FetchBalance(FetchBalanceCallback callback) override;

  void GetExternalWallets(
      ledger::GetExternalWalletsCallback callback) override;

  void GetExternalWallet(const std::string& wallet_type,
                         GetExternalWalletCallback callback) override;

  void ExternalWalletAuthorization(
      const std::string& wallet_type,
      const std::map<std::string, std::string>& args,
      ExternalWalletAuthorizationCallback callback);

  void ProcessRewardsPageUrl(
      const std::string& path,
      const std::string& query,
      ProcessRewardsPageUrlCallback callback) override;

  void DisconnectWallet(const std::string& wallet_type) override;

  void SaveExternalWallet(
      const std::string& wallet_type,
      ledger::ExternalWalletPtr wallet) override;

  bool OnlyAnonWallet() override;

  void GetAnonWalletStatus(GetAnonWalletStatusCallback callback) override;

  void SetAutoContribute(bool enabled) override;

  void GetMonthlyReport(
      const uint32_t month,
      const uint32_t year,
      GetMonthlyReportCallback callback) override;

  // Testing methods
  void SetLedgerEnvForTesting();
  void StartMonthlyContributionForTest();
  void MaybeShowNotificationAddFundsForTesting(
      base::OnceCallback<void(bool)> callback);
  void CheckInsufficientFundsForTesting();

 private:
  friend class ::BraveRewardsBrowserTest;
  FRIEND_TEST_ALL_PREFIXES(RewardsServiceTest, OnWalletProperties);

  const base::OneShotEvent& ready() const { return ready_; }

  void OnResult(ledger::ResultCallback callback, const ledger::Result result);

  void OnCreateWallet(CreateWalletCallback callback,
                      ledger::Result result);
  void OnLedgerStateSaved(ledger::LedgerCallbackHandler* handler,
                          bool success);
  void OnLedgerStateLoaded(ledger::OnLoadCallback callback,
                              std::pair<std::string, base::Value> data);
  void LoadNicewareList(ledger::GetNicewareListCallback callback) override;
  void OnPublisherStateSaved(ledger::LedgerCallbackHandler* handler,
                             bool success);
  void OnPublisherStateLoaded(ledger::OnLoadCallback callback,
                              const std::string& data);
  void OnFetchWalletProperties(const ledger::Result result,
                               ledger::WalletPropertiesPtr properties);
  void OnFetchPromotions(
    const ledger::Result result,
    ledger::PromotionList promotions);
  void TriggerOnPromotion(
      const ledger::Result result,
      ledger::PromotionPtr promotion);
  void TriggerOnRewardsMainEnabled(bool rewards_main_enabled);
  void OnPublisherInfoSaved(ledger::PublisherInfoCallback callback,
                            ledger::PublisherInfoPtr info,
                            bool success);
  void OnActivityInfoSaved(ledger::PublisherInfoCallback callback,
                            ledger::PublisherInfoPtr info,
                            bool success);
  void OnActivityInfoLoaded(ledger::PublisherInfoCallback callback,
                            const std::string& publisher_key,
                            ledger::PublisherInfoList list);
  void OnMediaPublisherInfoSaved(bool success);
  void OnPublisherInfoLoaded(ledger::PublisherInfoCallback callback,
                             ledger::PublisherInfoPtr info);
  void OnMediaPublisherInfoLoaded(ledger::PublisherInfoCallback callback,
                             ledger::PublisherInfoPtr info);
  void OnRestorePublishersUI(const ledger::Result result);
  void OnPublisherInfoListLoaded(uint32_t start,
                                 uint32_t limit,
                                 ledger::PublisherInfoListCallback callback,
                                 ledger::PublisherInfoList list);
  void OnTimer(uint32_t timer_id);
  void OnSavedState(ledger::OnSaveCallback callback, bool success);
  void OnLoadedState(ledger::OnLoadCallback callback,
                     const std::string& value);
  void OnResetState(ledger::OnResetCallback callback,
                                 bool success);
  void OnTipPublisherInfoSaved(
      const ledger::Result result,
      ledger::PublisherInfoPtr info,
      const bool recurring,
      const double amount);
  void OnTip(const std::string& publisher_key,
             double amount,
             bool recurring,
             ledger::PublisherInfoPtr publisher_info);
  void OnResetTheWholeState(base::Callback<void(bool)> callback,
                                 bool success);
  void OnContributionInfoSaved(
      ledger::ResultCallback callback,
      const ledger::Result result);

  void OnRecurringTipSaved(
      ledger::SaveRecurringTipCallback callback,
      const bool success);
  void OnGetRecurringTips(
      const ledger::PublisherInfoListCallback callback,
      ledger::PublisherInfoList list);
  void OnGetOneTimeTips(ledger::PublisherInfoListCallback callback,
                        ledger::PublisherInfoList list);
  void OnRecurringTipUI(const ledger::Result result);
  void OnRemoveRecurringTip(ledger::RemoveRecurringTipCallback callback,
                            const bool success);
  void RemoveRecurringTip(const std::string& publisher_key,
                         ledger::RemoveRecurringTipCallback callback) override;
  void TriggerOnGetCurrentBalanceReport(
      ledger::BalanceReportInfoPtr report);
  void MaybeShowBackupNotification(uint64_t boot_stamp);
  void MaybeShowAddFundsNotification(uint64_t reconcile_stamp);
  void OnPublisherListNormalizedSaved(ContentSiteList site_list,
                                      bool success);
  void OnWalletProperties(
      const ledger::Result result,
      ledger::WalletPropertiesPtr properties) override;

  void DeleteActivityInfo(
    const std::string& publisher_key,
    ledger::DeleteActivityInfoCallback callback) override;

  void OnDeleteActivityInfoStamp(
      const std::string& publisher_key,
      const ledger::DeleteActivityInfoCallback& callback,
      uint64_t reconcile_stamp);

  void OnDeleteActivityInfo(
    const std::string& publisher_key,
    const ledger::DeleteActivityInfoCallback& callback,
    bool result);

  void OnGetOneTimeTipsUI(GetRecurringTipsCallback callback,
                          ledger::PublisherInfoList list);

  void OnPublisherActivityInfoLoaded(ledger::PublisherInfoCallback callback,
                                     const ledger::Result result,
                                     ledger::PublisherInfoPtr info);

  void OnInlineTipSetting(GetInlineTipSettingCallback callback, bool enabled);

  void OnShareURL(GetShareURLCallback callback, const std::string& url);

  void OnPendingContributionRemoved(
    ledger::RemovePendingContributionCallback callback,
    bool result);

  void OnRemoveAllPendingContribution(
    ledger::RemovePendingContributionCallback callback,
    bool result);

  void OnGetPendingContributionsUI(
    GetPendingContributionsCallback callback,
    ledger::PendingContributionInfoList list);

  void OnGetPendingContributions(
    ledger::PendingContributionInfoListCallback callback,
    ledger::PendingContributionInfoList list);

  void OnURLLoaderComplete(network::SimpleURLLoader* loader,
                           ledger::LoadURLCallback callback,
                           std::unique_ptr<std::string> response_body);

  void StartNotificationTimers(bool main_enabled);
  void StopNotificationTimers();
  void OnNotificationTimerFired();

  void MaybeShowNotificationAddFunds();
  bool ShouldShowNotificationAddFunds() const;
  void ShowNotificationAddFunds(bool sufficient);

  void OnMaybeShowNotificationAddFundsForTesting(
      base::OnceCallback<void(bool)> callback,
      const bool result);

  void MaybeShowNotificationTipsPaid();
  void ShowNotificationTipsPaid(bool ac_enabled);

  void OnPendingContributionRemovedUI(const ledger::Result result);

  void OnRemoveAllPendingContributionsUI(const ledger::Result result);

  void OnGetPendingContributionsTotal(
    ledger::PendingContributionsTotalCallback callback,
    double amount);

  void OnFetchBalance(FetchBalanceCallback callback,
                      const ledger::Result result,
                      ledger::BalancePtr balance);

  void OnGetExternalWallet(
    const std::string& wallet_type,
    GetExternalWalletCallback callback,
    const ledger::Result result,
    ledger::ExternalWalletPtr wallet);

  void OnExternalWalletAuthorization(
    const std::string& wallet_type,
    ExternalWalletAuthorizationCallback callback,
    const ledger::Result result,
    const base::flat_map<std::string, std::string>& args);

  void OnProcessExternalWalletAuthorization(
    const std::string& wallet_type,
    const std::string& action,
    ProcessRewardsPageUrlCallback callback,
    const ledger::Result result,
    const std::map<std::string, std::string>& args);

  void OnDisconnectWallet(
    const std::string& wallet_type,
    const ledger::Result result);

  void OnSetPublisherExclude(const std::string& publisher_key,
                             const bool exclude,
                             const ledger::Result result);

  void OnWalletInitialized(ledger::Result result);

  void OnClaimPromotion(
      ClaimPromotionCallback callback,
      const ledger::Result result,
      const std::string& response);

  void AttestationAndroid(
      const std::string& promotion_id,
      AttestPromotionCallback callback,
      const ledger::Result result,
      const std::string& response);

  void OnAttestationAndroid(
      const std::string& promotion_id,
      AttestPromotionCallback callback,
      const std::string& nonce,
      bool result,
      const std::string& token);

  void OnGetAnonWalletStatus(
      GetAnonWalletStatusCallback callback,
      const ledger::Result result);

  // ledger::LedgerClient
  std::string GenerateGUID() const override;
  void OnRecoverWallet(
      ledger::Result result,
      double balance);
  void OnReconcileComplete(
      const ledger::Result result,
      const std::string& viewing_id,
      const double amount,
      const ledger::RewardsType type) override;
  void OnAttestPromotion(
      AttestPromotionCallback callback,
      const ledger::Result result,
      ledger::PromotionPtr promotion);
  void LoadLedgerState(ledger::OnLoadCallback callback) override;
  void LoadPublisherState(ledger::OnLoadCallback callback) override;
  void SaveLedgerState(const std::string& ledger_state,
                       ledger::LedgerCallbackHandler* handler) override;
  void SavePublisherState(const std::string& publisher_state,
                          ledger::LedgerCallbackHandler* handler) override;
  void SavePublisherInfo(ledger::PublisherInfoPtr publisher_info,
                         ledger::PublisherInfoCallback callback) override;
  void SaveActivityInfo(ledger::PublisherInfoPtr publisher_info,
                        ledger::PublisherInfoCallback callback) override;
  void LoadActivityInfo(ledger::ActivityInfoFilterPtr filter,
                         ledger::PublisherInfoCallback callback) override;
  void LoadPanelPublisherInfo(ledger::ActivityInfoFilterPtr filter,
                              ledger::PublisherInfoCallback callback) override;
  void GetActivityInfoList(
      uint32_t start,
      uint32_t limit,
      ledger::ActivityInfoFilterPtr filter,
      ledger::PublisherInfoListCallback callback) override;
  void SetTimer(uint64_t time_offset, uint32_t* timer_id) override;
  void LoadURL(const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const ledger::UrlMethod method,
      ledger::LoadURLCallback callback) override;
  void SetRewardsMainEnabled(bool enabled) override;
  void SetPublisherMinVisits(unsigned int visits) const override;
  void SetPublisherAllowNonVerified(bool allow) const override;
  void SetPublisherAllowVideos(bool allow) const override;
  void SetUserChangedContribution() const override;
  void UpdateAdsRewards() const override;
  void SetCatalogIssuers(const std::string& json) override;
  void ConfirmAd(const std::string& json) override;
  void ConfirmAction(const std::string& uuid,
                     const std::string& creative_set_id,
                     const std::string& type) override;
  void SetConfirmationsIsReady(const bool is_ready) override;
  void GetTransactionHistory(
      GetTransactionHistoryCallback callback) override;
  void ConfirmationsTransactionHistoryDidChange() override;

  void OnPanelPublisherInfo(const ledger::Result result,
                            ledger::PublisherInfoPtr info,
                            uint64_t window_id) override;
  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    ledger::FetchIconCallback callback) override;
  void OnFetchFavIconCompleted(ledger::FetchIconCallback callback,
                          const std::string& favicon_key,
                          const GURL& url,
                          const BitmapFetcherService::RequestId& request_id,
                          const SkBitmap& image);
  void OnSetOnDemandFaviconComplete(const std::string& favicon_url,
                                    ledger::FetchIconCallback callback,
                                    bool success);
  void SaveContributionInfo(
      ledger::ContributionInfoPtr info,
      ledger::ResultCallback callback) override;

  void SaveRecurringTip(
      ledger::RecurringTipPtr info,
      ledger::SaveRecurringTipCallback callback) override;

  void GetRecurringTips(
      ledger::PublisherInfoListCallback callback) override;
  std::unique_ptr<ledger::LogStream> Log(
                     const char* file,
                     int line,
                     const ledger::LogLevel log_level) const override;

  std::unique_ptr<ledger::LogStream> VerboseLog(
                     const char* file,
                     int line,
                     int log_level) const override;
  void SaveState(const std::string& name,
                 const std::string& value,
                 ledger::OnSaveCallback callback) override;
  void LoadState(const std::string& name,
                 ledger::OnLoadCallback callback) override;
  void ResetState(const std::string& name,
                  ledger::OnResetCallback callback) override;
  void SetBooleanState(const std::string& name, bool value) override;
  bool GetBooleanState(const std::string& name) const override;
  void SetIntegerState(const std::string& name, int value) override;
  int GetIntegerState(const std::string& name) const override;
  void SetDoubleState(const std::string& name, double value) override;
  double GetDoubleState(const std::string& name) const override;
  void SetStringState(const std::string& name,
                      const std::string& value) override;
  std::string GetStringState(const std::string& name) const override;
  void SetInt64State(const std::string& name, int64_t value) override;
  int64_t GetInt64State(const std::string& name) const override;
  void SetUint64State(const std::string& name, uint64_t value) override;
  uint64_t GetUint64State(const std::string& name) const override;
  void ClearState(const std::string& name) override;

  bool GetBooleanOption(const std::string& name) const override;
  int GetIntegerOption(const std::string& name) const override;
  double GetDoubleOption(const std::string& name) const override;
  std::string GetStringOption(const std::string& name) const override;
  int64_t GetInt64Option(const std::string& name) const override;
  uint64_t GetUint64Option(const std::string& name) const override;


  void KillTimer(uint32_t timer_id) override;

  void RestorePublishers(ledger::RestorePublishersCallback callback) override;

  void OnPanelPublisherInfoLoaded(
      ledger::PublisherInfoCallback callback,
      ledger::PublisherInfoPtr publisher_info);

  void SavePendingContribution(
      ledger::PendingContributionList list,
      ledger::SavePendingContributionCallback callback) override;

  void OnSavePendingContribution(
      ledger::SavePendingContributionCallback callback,
      ledger::Result result);

  void OnRestorePublishers(ledger::RestorePublishersCallback callback,
                           const bool success);

  void SaveNormalizedPublisherList(
      ledger::PublisherInfoList list) override;

  void GetPendingContributions(
    ledger::PendingContributionInfoListCallback callback) override;

  void RemovePendingContribution(
    const uint64_t id,
    ledger::RemovePendingContributionCallback callback) override;

  void RemoveAllPendingContributions(
      ledger::RemovePendingContributionCallback callback) override;

  void GetPendingContributionsTotal(
      ledger::PendingContributionsTotalCallback callback) override;

  void ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      ledger::ShowNotificationCallback callback) override;

  void ClearAndInsertServerPublisherList(
      ledger::ServerPublisherInfoList list,
      ledger::ClearAndInsertServerPublisherListCallback callback) override;

  void GetServerPublisherInfo(
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback) override;

  void SetTransferFee(
      const std::string& wallet_type,
      ledger::TransferFeePtr transfer_fee) override;

  ledger::TransferFeeList GetTransferFees(
      const std::string& wallet_type) override;

  void RemoveTransferFee(
      const std::string& wallet_type,
      const std::string& id) override;

  void InsertOrUpdateContributionQueue(
    ledger::ContributionQueuePtr info,
    ledger::ResultCallback callback) override;

  void DeleteContributionQueue(
    const uint64_t id,
    ledger::ResultCallback callback) override;

  void GetFirstContributionQueue(
    ledger::GetFirstContributionQueueCallback callback) override;

  void InsertOrUpdatePromotion(
      ledger::PromotionPtr info,
      ledger::ResultCallback callback) override;

  void GetPromotion(
      const std::string& id,
      ledger::GetPromotionCallback callback) override;

  void GetAllPromotions(
      ledger::GetAllPromotionsCallback callback) override;

  void InsertOrUpdateUnblindedToken(
      ledger::UnblindedTokenPtr info,
      ledger::ResultCallback callback) override;

  void GetAllUnblindedTokens(
      ledger::GetAllUnblindedTokensCallback callback) override;

  void DeleteUnblindedTokens(
    const std::vector<std::string>& id_list,
    ledger::ResultCallback callback) override;

  void DeleteUnblindedTokensForPromotion(
      const std::string& promotion_id,
      ledger::ResultCallback callback) override;

  ledger::ClientInfoPtr GetClientInfo() override;

  void UnblindedTokensReady() override;

  void GetTransactionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetTransactionReportCallback callback) override;

  void GetContributionReport(
      const ledger::ActivityMonth month,
      const int year,
      ledger::GetContributionReportCallback callback) override;

  void GetIncompleteContributions(
      ledger::GetIncompleteContributionsCallback callback) override;

  void GetContributionInfo(
      const std::string& contribution_id,
      ledger::GetContributionInfoCallback callback) override;

  void UpdateContributionInfoStepAndCount(
      const std::string& contribution_id,
      const ledger::ContributionStep step,
      const int32_t retry_count,
      ledger::ResultCallback callback) override;

  void UpdateContributionInfoContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ledger::ResultCallback callback) override;

  void ReconcileStampReset() override;

  // end ledger::LedgerClient

  // Mojo Proxy methods
  void OnGetTransactionHistory(
      GetTransactionHistoryCallback callback,
      const std::string& transactions);
  void OnGetAutoContributeProps(
      const GetAutoContributePropsCallback& callback,
      ledger::AutoContributePropsPtr props);
  void OnGetRewardsInternalsInfo(GetRewardsInternalsInfoCallback callback,
                                 ledger::RewardsInternalsInfoPtr info);
  void SetRewardsMainEnabledPref(bool enabled);
  void SetRewardsMainEnabledMigratedPref(bool enabled);
  void OnRefreshPublisher(
      RefreshPublisherCallback callback,
      const std::string& publisher_key,
      ledger::PublisherStatus status);
  void OnMediaInlineInfoSaved(
      SaveMediaInfoCallback callback,
      const ledger::Result result,
      ledger::PublisherInfoPtr publisher);

  void OnContributeUnverifiedPublishers(
      ledger::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) override;

  void OnClearAndInsertServerPublisherList(
    ledger::ClearAndInsertServerPublisherListCallback callback,
    bool result);

  void OnGetServerPublisherInfo(
    ledger::GetServerPublisherInfoCallback callback,
    ledger::ServerPublisherInfoPtr info);

  bool Connected() const;
  void ConnectionClosed();
  void AddPrivateObserver(RewardsServicePrivateObserver* observer) override;
  void RemovePrivateObserver(RewardsServicePrivateObserver* observer) override;

  void RecordBackendP3AStats() const;

  void OnGetFirstContributionQueue(
    ledger::GetFirstContributionQueueCallback callback,
    ledger::ContributionQueuePtr info);

  void OnGetPromotion(
      ledger::GetPromotionCallback callback,
      ledger::PromotionPtr info);

  void OnGetAllUnblindedTokens(
      ledger::GetAllUnblindedTokensCallback callback,
      ledger::UnblindedTokenList list);

  void OnGetAllPromotions(
      ledger::GetAllPromotionsCallback callback,
      ledger::PromotionMap promotions);

  void OnGetBalanceReport(
      GetBalanceReportCallback callback,
      const ledger::Result result,
      ledger::BalanceReportInfoPtr report);

  void OnGetMonthlyReportBalance(
      const uint32_t month,
      const uint32_t year,
      GetMonthlyReportCallback callback,
      const ledger::Result result,
      ledger::BalanceReportInfoPtr report);

  void OnGetMonthlyReportTransaction(
      const uint32_t month,
      const uint32_t year,
      const MonthlyReport& report,
      GetMonthlyReportCallback callback,
      ledger::TransactionReportInfoList list);

  void OnGetMonthlyReportContribution(
      const MonthlyReport& report,
      GetMonthlyReportCallback callback,
      ledger::ContributionReportInfoList list);

  void OnGetTransactionReport(
      ledger::GetTransactionReportCallback callback,
      ledger::TransactionReportInfoList list);

  void OnGetContributionReport(
      ledger::GetContributionReportCallback callback,
      ledger::ContributionReportInfoList list);

  void OnGetNotCompletedContributions(
      ledger::GetIncompleteContributionsCallback callback,
      ledger::ContributionInfoList list);

  void OnGetContributionInfo(
      ledger::GetContributionInfoCallback callback,
      ledger::ContributionInfoPtr info);

#if defined(OS_ANDROID)
  ledger::Environment GetServerEnvironmentForAndroid();
  void CreateWalletAttestationResult(
      bat_ledger::mojom::BatLedger::CreateWalletCallback callback,
      bool result, const std::string& result_string);
  void GrantAttestationResult(
      const std::string& promotion_id, bool result,
      const std::string& result_string);
  safetynet_check::SafetyNetCheckRunner safetynet_check_runner_;
#endif

  Profile* profile_;  // NOT OWNED
  mojo::AssociatedBinding<bat_ledger::mojom::BatLedgerClient>
      bat_ledger_client_binding_;
  bat_ledger::mojom::BatLedgerAssociatedPtr bat_ledger_;
  mojo::Remote<bat_ledger::mojom::BatLedgerService> bat_ledger_service_;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<ExtensionRewardsServiceObserver>
      extension_rewards_service_observer_;
#endif
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  const base::FilePath ledger_state_path_;
  const base::FilePath publisher_state_path_;
  const base::FilePath publisher_info_db_path_;
  const base::FilePath publisher_list_path_;
  const base::FilePath rewards_base_path_;
  std::unique_ptr<PublisherInfoDatabase> publisher_info_backend_;
  std::unique_ptr<RewardsNotificationServiceImpl> notification_service_;
  base::ObserverList<RewardsServicePrivateObserver> private_observers_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<ExtensionRewardsServiceObserver> private_observer_;
#endif

  base::OneShotEvent ready_;
  base::flat_set<network::SimpleURLLoader*> url_loaders_;
  std::map<uint32_t, std::unique_ptr<base::OneShotTimer>> timers_;
  std::vector<std::string> current_media_fetchers_;
  std::vector<BitmapFetcherService::RequestId> request_ids_;
  std::unique_ptr<base::OneShotTimer> notification_startup_timer_;
  std::unique_ptr<base::RepeatingTimer> notification_periodic_timer_;

  uint32_t next_timer_id_;
  bool reset_states_;

  GetTestResponseCallback test_response_callback_;

  // At the moment we keep it only for the purpose of sending P3A stats.
  // Used only on UI thread.
  bool auto_contributions_enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(RewardsServiceImpl);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_
