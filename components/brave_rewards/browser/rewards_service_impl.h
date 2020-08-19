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
#include "base/files/file.h"
#include "base/observer_list.h"
#include "base/one_shot_event.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "ui/gfx/image/image.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"
#include "brave/components/brave_rewards/browser/rewards_service_private_observer.h"

#if defined(OS_ANDROID)
#include "brave/components/safetynet/safetynet_check.h"
#endif

namespace base {
class OneShotTimer;
class RepeatingTimer;
class SequencedTaskRunner;
}  // namespace base

namespace ledger {
class Ledger;
struct LedgerMediaPublisherInfo;
}  // namespace ledger

namespace leveldb {
class DB;
}  // namespace leveldb

namespace network {
class SimpleURLLoader;
}  // namespace network

#if BUILDFLAG(ENABLE_GREASELION)
namespace greaselion {
class GreaselionService;
}  // namespace greaselion
#endif

class Profile;
class RewardsFlagBrowserTest;

namespace brave_rewards {

class RewardsDatabase;
class RewardsNotificationServiceImpl;
class RewardsBrowserTest;

using GetEnvironmentCallback = base::Callback<void(ledger::Environment)>;
using GetDebugCallback = base::Callback<void(bool)>;
using GetReconcileIntervalCallback = base::Callback<void(int32_t)>;
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

using StopLedgerCallback = base::OnceCallback<void(ledger::Result)>;

class RewardsServiceImpl : public RewardsService,
                           public ledger::LedgerClient,
                           public base::SupportsWeakPtr<RewardsServiceImpl> {
 public:
#if BUILDFLAG(ENABLE_GREASELION)
  explicit RewardsServiceImpl(
      Profile* profile,
      greaselion::GreaselionService* greaselion_service);
#else
  explicit RewardsServiceImpl(Profile* profile);
#endif
  ~RewardsServiceImpl() override;

  // KeyedService:
  void Shutdown() override;

  bool IsInitialized() override;

  void Init(
      std::unique_ptr<RewardsServiceObserver> extension_observer,
      std::unique_ptr<RewardsServicePrivateObserver> private_observer,
      std::unique_ptr<RewardsNotificationServiceObserver>
          notification_observer);
  void StartLedger();
  void CreateWallet(CreateWalletCallback callback) override;
  void GetRewardsParameters(GetRewardsParametersCallback callback) override;
  void FetchPromotions() override;
  void ClaimPromotion(
      const std::string& promotion_id,
      ClaimPromotionCallback callback) override;
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
      const GetContentSiteListCallback& callback) override;

  void GetExcludedList(const GetContentSiteListCallback& callback) override;

  void OnGetContentSiteList(
      const GetContentSiteListCallback& callback,
      ledger::PublisherInfoList list);
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
  void GetAutoContributeEnabled(
      GetAutoContributeEnabledCallback callback) override;
  void GetPublisherMinVisitTime(
      const GetPublisherMinVisitTimeCallback& callback) override;
  void GetPublisherMinVisits(
      const GetPublisherMinVisitsCallback& callback) override;
  void GetPublisherAllowNonVerified(
      const GetPublisherAllowNonVerifiedCallback& callback) override;
  void GetPublisherAllowVideos(
      const GetPublisherAllowVideosCallback& callback) override;
  void RestorePublishers() override;
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
  void GetAutoContributionAmount(
      const GetAutoContributionAmountCallback& callback) override;
  void GetPublisherBanner(const std::string& publisher_id,
                          GetPublisherBannerCallback callback) override;
  void OnPublisherBanner(GetPublisherBannerCallback callback,
                         ledger::PublisherBannerPtr banner);
  void RemoveRecurringTip(const std::string& publisher_key) override;
  void OnGetRecurringTips(
      GetRecurringTipsCallback callback,
      ledger::PublisherInfoList list);
  void GetRecurringTips(GetRecurringTipsCallback callback) override;
  void SetPublisherExclude(
      const std::string& publisher_key,
      bool exclude) override;
  RewardsNotificationService* GetNotificationService() const override;
  void SetBackupCompleted() override;
  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;

  void HandleFlags(const std::string& options);
  void SetEnvironment(ledger::Environment environment);
  void GetEnvironment(const GetEnvironmentCallback& callback);
  void SetDebug(bool debug);
  void GetDebug(const GetDebugCallback& callback);
  void SetReconcileInterval(const int32_t interval);
  void GetReconcileInterval(GetReconcileIntervalCallback callback);
  void SetShortRetries(bool short_retries);
  void GetShortRetries(const GetShortRetriesCallback& callback);

  void GetAutoContributeProperties(
      const GetAutoContributePropertiesCallback& callback) override;
  void GetPendingContributionsTotal(
      const GetPendingContributionsTotalCallback& callback) override;
  void GetRewardsMainEnabled(
      const GetRewardsMainEnabledCallback& callback) const override;

  void GetOneTimeTips(GetOneTimeTipsCallback callback) override;
  void RefreshPublisher(
      const std::string& publisher_key,
      RefreshPublisherCallback callback) override;
  void OnAdsEnabled(bool ads_enabled) override;

  void OnSaveRecurringTip(
      SaveRecurringTipCallback callback,
      const ledger::Result result);
  void SaveRecurringTip(
      const std::string& publisher_key,
      const double amount,
      SaveRecurringTipCallback callback) override;

  const RewardsNotificationService::RewardsNotificationsMap&
    GetAllNotifications() override;

  void SetAutoContributionAmount(const double amount) const override;

  void SaveInlineMediaInfo(
      const std::string& media_type,
      const std::map<std::string, std::string>& args,
      SaveMediaInfoCallback callback) override;

  void SetInlineTippingPlatformEnabled(
      const std::string& key,
      bool enabled) override;

  void GetInlineTippingPlatformEnabled(
      const std::string& key,
      GetInlineTippingPlatformEnabledCallback callback) override;

  void GetShareURL(
      const std::string& type,
      const std::map<std::string, std::string>& args,
      GetShareURLCallback callback) override;

  void GetPendingContributions(
    GetPendingContributionsCallback callback) override;

  void RemovePendingContribution(const uint64_t id) override;

  void RemoveAllPendingContributions() override;

  void OnTip(
      const std::string& publisher_key,
      const double amount,
      const bool recurring,
      std::unique_ptr<brave_rewards::ContentSite> site) override;

  void OnTip(
      const std::string& publisher_key,
      const double amount,
      const bool recurring) override;

  void SetPublisherMinVisitTime(int duration_in_seconds) const override;

  void FetchBalance(FetchBalanceCallback callback) override;

  std::map<std::string, ledger::ExternalWalletPtr>
  GetExternalWallets() override;

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

  void SetAutoContributeEnabled(bool enabled) override;

  void GetMonthlyReport(
      const uint32_t month,
      const uint32_t year,
      GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(GetAllMonthlyReportIdsCallback callback) override;

  void GetAllContributions(GetAllContributionsCallback callback) override;

  void GetAllPromotions(GetAllPromotionsCallback callback) override;

  // Testing methods
  void SetLedgerEnvForTesting();
  void PrepareLedgerEnvForTesting();
  void StartMonthlyContributionForTest();
  void MaybeShowNotificationAddFundsForTesting(
      base::OnceCallback<void(bool)> callback);
  void CheckInsufficientFundsForTesting();
  ledger::TransferFeeList GetTransferFeesForTesting(
      const std::string& wallet_type);
  bool IsWalletInitialized();
  void ForTestingSetTestResponseCallback(GetTestResponseCallback callback);

 private:
  friend class ::RewardsFlagBrowserTest;

  void EnableGreaseLion(const bool enabled);

  void StopLedger(StopLedgerCallback callback);

  void OnStopLedger(
      StopLedgerCallback callback,
      const ledger::Result result);
  void OnStopLedgerForCompleteReset(
      SuccessCallback callback,
      const ledger::Result result);

  void Reset();

  bool ResetOnFilesTaskRunner();

  void OnCreate();

  void OnResult(ledger::ResultCallback callback, const ledger::Result result);

  void OnCreateWallet(CreateWalletCallback callback,
                      ledger::Result result);
  void OnLedgerStateLoaded(ledger::OnLoadCallback callback,
                              std::pair<std::string, base::Value> data);
  void OnPublisherStateLoaded(ledger::OnLoadCallback callback,
                              const std::string& data);
  void OnGetRewardsParameters(
      GetRewardsParametersCallback callback,
      ledger::RewardsParametersPtr properties);
  void OnFetchPromotions(
    const ledger::Result result,
    ledger::PromotionList promotions);
  void TriggerOnPromotion(
      const ledger::Result result,
      ledger::PromotionPtr promotion);
  void TriggerOnRewardsMainEnabled(bool rewards_main_enabled);
  void OnRestorePublishers(const ledger::Result result);
  void OnPublisherInfoListLoaded(uint32_t start,
                                 uint32_t limit,
                                 ledger::PublisherInfoListCallback callback,
                                 ledger::PublisherInfoList list);
  void OnTimer(uint32_t timer_id);
  void OnSavedState(ledger::ResultCallback callback, bool success);
  void OnLoadedState(ledger::OnLoadCallback callback,
                     const std::string& value);
  void OnResetState(ledger::ResultCallback callback,
                                 bool success);
  void OnTipPublisherInfoSaved(
      const ledger::Result result,
      ledger::PublisherInfoPtr info,
      const bool recurring,
      const double amount);

  void OnRecurringTip(const ledger::Result result);

  void TriggerOnGetCurrentBalanceReport(
      ledger::BalanceReportInfoPtr report);
  void MaybeShowBackupNotification(uint64_t boot_stamp);
  void MaybeShowAddFundsNotification(uint64_t reconcile_stamp);

  void OnGetOneTimeTips(
      GetRecurringTipsCallback callback,
      ledger::PublisherInfoList list);

  void OnInlineTipSetting(
      GetInlineTippingPlatformEnabledCallback callback,
      bool enabled);

  void OnShareURL(GetShareURLCallback callback, const std::string& url);

  void OnGetPendingContributions(
    GetPendingContributionsCallback callback,
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

  void OnPendingContributionRemoved(const ledger::Result result);

  void OnRemoveAllPendingContributions(const ledger::Result result);

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
      const bool token_received,
      const std::string& token,
      const bool attestation_passed);

  void OnGetAnonWalletStatus(
      GetAnonWalletStatusCallback callback,
      const ledger::Result result);

  void OnRecoverWallet(const ledger::Result result);

  // ledger::LedgerClient
  void OnReconcileComplete(
      const ledger::Result result,
      ledger::ContributionInfoPtr contribution) override;
  void OnAttestPromotion(
      AttestPromotionCallback callback,
      const ledger::Result result,
      ledger::PromotionPtr promotion);
  void LoadLedgerState(ledger::OnLoadCallback callback) override;
  void LoadPublisherState(ledger::OnLoadCallback callback) override;
  void SetTimer(uint64_t time_offset, uint32_t* timer_id) override;
  void LoadURL(const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& contentType,
      const ledger::UrlMethod method,
      ledger::LoadURLCallback callback) override;
  void SetRewardsMainEnabled(bool enabled) override;
  void SetPublisherMinVisits(int visits) const override;
  void SetPublisherAllowNonVerified(bool allow) const override;
  void SetPublisherAllowVideos(bool allow) const override;
  void UpdateAdsRewards() const override;
  void SetCatalogIssuers(
      const std::string& json) override;
  void ConfirmAd(
      const std::string& json,
      const std::string& confirmation_type) override;
  void ConfirmAction(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
      const std::string& confirmation_type) override;
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
                          const SkBitmap& image);
  void OnSetOnDemandFaviconComplete(const std::string& favicon_url,
                                    ledger::FetchIconCallback callback,
                                    bool success);

  bool MaybeTailDiagnosticLog(
      const int num_lines);

  void DiagnosticLog(
      const std::string& file,
      const int line,
      const int verbose_level,
      const std::string& message) override;

  bool WriteToDiagnosticLogOnFileTaskRunner(
      const base::FilePath& log_path,
      const int num_lines,
      const std::string& file,
      const int line,
      const int verbose_level,
      const std::string& message);

  void OnWriteToLogOnFileTaskRunner(
    const bool success);

  void LoadDiagnosticLog(
      const int num_lines,
      LoadDiagnosticLogCallback callback) override;

  std::string LoadDiagnosticLogOnFileTaskRunner(
      const base::FilePath& path,
      const int num_lines);

  void OnLoadDiagnosticLogOnFileTaskRunner(
      LoadDiagnosticLogCallback callback,
      const std::string& value);

  void ClearDiagnosticLog(ClearDiagnosticLogCallback callback) override;

  void CompleteReset(SuccessCallback callback) override;

  bool ClearDiagnosticLogOnFileTaskRunner(
      const base::FilePath& path);

  void OnClearDiagnosticLogOnFileTaskRunner(
      ClearDiagnosticLogCallback callback,
      const bool success);

  void Log(
      const char* file,
      const int line,
      const int verbose_level,
      const std::string& message) override;

  void SaveState(const std::string& name,
                 const std::string& value,
                 ledger::ResultCallback callback) override;
  void LoadState(const std::string& name,
                 ledger::OnLoadCallback callback) override;
  void ResetState(const std::string& name,
                  ledger::ResultCallback callback) override;
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

  void PublisherListNormalized(ledger::PublisherInfoList list) override;

  void ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      ledger::ResultCallback callback) override;

  void SetTransferFee(
      const std::string& wallet_type,
      ledger::TransferFeePtr transfer_fee) override;

  ledger::TransferFeeList GetTransferFees(
      const std::string& wallet_type) override;

  void RemoveTransferFee(
      const std::string& wallet_type,
      const std::string& id) override;

  ledger::ClientInfoPtr GetClientInfo() override;

  void UnblindedTokensReady() override;

  void ReconcileStampReset() override;

  void RunDBTransaction(
      ledger::DBTransactionPtr transaction,
      ledger::RunDBTransactionCallback callback) override;

  void GetCreateScript(
      ledger::GetCreateScriptCallback callback) override;

  void PendingContributionSaved(const ledger::Result result) override;

  void OnTipPublisherSaved(
      const std::string& publisher_key,
      const double amount,
      const bool recurring,
      const ledger::Result result);

  void ClearAllNotifications() override;

  void DeleteLog(ledger::ResultCallback callback) override;

  // end ledger::LedgerClient

  // Mojo Proxy methods
  void OnGetTransactionHistory(
      GetTransactionHistoryCallback callback,
      const std::string& json);
  void OnGetAutoContributeProperties(
      const GetAutoContributePropertiesCallback& callback,
      ledger::AutoContributePropertiesPtr props);
  void OnGetRewardsInternalsInfo(GetRewardsInternalsInfoCallback callback,
                                 ledger::RewardsInternalsInfoPtr info);

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

  bool Connected() const;
  void ConnectionClosed();
  void AddPrivateObserver(RewardsServicePrivateObserver* observer) override;
  void RemovePrivateObserver(RewardsServicePrivateObserver* observer) override;

  void RecordBackendP3AStats();

  void OnRecordBackendP3AStatsRecurring(ledger::PublisherInfoList list);

  void OnRecordBackendP3AStatsContributions(
      const uint32_t recurring_donation_size,
      ledger::ContributionInfoList list);

  void OnRecordBackendP3AStatsAC(
      const int auto_contributions,
      bool ac_enabled);

  void OnGetBalanceReport(
      GetBalanceReportCallback callback,
      const ledger::Result result,
      ledger::BalanceReportInfoPtr report);

  void OnGetMonthlyReport(
      GetMonthlyReportCallback callback,
      const ledger::Result result,
      ledger::MonthlyReportInfoPtr report);

  void OnRunDBTransaction(
      ledger::RunDBTransactionCallback callback,
      ledger::DBCommandResponsePtr response);

  void OnGetAllMonthlyReportIds(
      GetAllMonthlyReportIdsCallback callback,
      const std::vector<std::string>& ids);

  void OnGetAllContributions(
      GetAllContributionsCallback callback,
      ledger::ContributionInfoList contributions);

  void OnGetAllPromotions(
      GetAllPromotionsCallback callback,
      base::flat_map<std::string, ledger::PromotionPtr> promotions);

  void OnCompleteReset(SuccessCallback callback, const bool success);

  bool DeleteLogTaskRunner();

  void OnDeleteLog(ledger::ResultCallback callback, const bool success);

#if defined(OS_ANDROID)
  ledger::Environment GetServerEnvironmentForAndroid();
  void CreateWalletAttestationResult(
      bat_ledger::mojom::BatLedger::CreateWalletCallback callback,
      const bool token_received,
      const std::string& result_string,
      const bool attestation_passed);
  void GrantAttestationResult(
      const std::string& promotion_id, bool result,
      const std::string& result_string);
  safetynet_check::SafetyNetCheckRunner safetynet_check_runner_;
#endif

  Profile* profile_;  // NOT OWNED
#if BUILDFLAG(ENABLE_GREASELION)
  greaselion::GreaselionService* greaselion_service_;  // NOT OWNED
#endif
  mojo::AssociatedReceiver<bat_ledger::mojom::BatLedgerClient>
      bat_ledger_client_receiver_;
  mojo::AssociatedRemote<bat_ledger::mojom::BatLedger> bat_ledger_;
  mojo::Remote<bat_ledger::mojom::BatLedgerService> bat_ledger_service_;
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  const base::FilePath diagnostic_log_path_;
  base::File diagnostic_log_;
  const base::FilePath ledger_state_path_;
  const base::FilePath publisher_state_path_;
  const base::FilePath publisher_info_db_path_;
  const base::FilePath publisher_list_path_;
  const base::FilePath rewards_base_path_;
  std::unique_ptr<RewardsDatabase> rewards_database_;
  std::unique_ptr<RewardsNotificationServiceImpl> notification_service_;
  base::ObserverList<RewardsServicePrivateObserver> private_observers_;
  std::unique_ptr<RewardsServiceObserver> extension_observer_;
  std::unique_ptr<RewardsServicePrivateObserver> private_observer_;

  std::unique_ptr<base::OneShotEvent> ready_;
  base::flat_set<network::SimpleURLLoader*> url_loaders_;
  std::map<uint32_t, std::unique_ptr<base::OneShotTimer>> timers_;
  std::map<std::string, BitmapFetcherService::RequestId>
      current_media_fetchers_;
  std::unique_ptr<base::OneShotTimer> notification_startup_timer_;
  std::unique_ptr<base::RepeatingTimer> notification_periodic_timer_;

  uint32_t next_timer_id_;
  bool reset_states_;
  bool is_wallet_initialized_ = false;
  bool ledger_for_testing_ = false;
  bool resetting_rewards_ = false;
  bool should_persist_logs_ = false;

  GetTestResponseCallback test_response_callback_;

  DISALLOW_COPY_AND_ASSIGN(RewardsServiceImpl);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_
