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
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/one_shot_event.h"
#include "base/values.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/brave_rewards/browser/publisher_info.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_private_observer.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/gfx/image/image.h"

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
class LedgerDatabase;
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

class RewardsNotificationServiceImpl;
class RewardsBrowserTest;

using GetEnvironmentCallback = base::Callback<void(ledger::type::Environment)>;
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
        const ledger::type::Result,
        const std::map<std::string, std::string>&)>;

using StopLedgerCallback = base::OnceCallback<void(ledger::type::Result)>;

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
      ledger::type::PublisherInfoList list);
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
                         ledger::type::PublisherBannerPtr banner);
  void RemoveRecurringTip(const std::string& publisher_key) override;
  void OnGetRecurringTips(
      GetRecurringTipsCallback callback,
      ledger::type::PublisherInfoList list);
  void GetRecurringTips(GetRecurringTipsCallback callback) override;
  void SetPublisherExclude(
      const std::string& publisher_key,
      bool exclude) override;
  RewardsNotificationService* GetNotificationService() const override;
  void SetBackupCompleted() override;
  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;

  void HandleFlags(const std::string& options);
  void SetEnvironment(ledger::type::Environment environment);
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
      const ledger::type::Result result);
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

  void UpdateMediaDuration(
      const uint64_t window_id,
      const std::string& publisher_key,
      uint64_t duration) override;

  void GetPublisherInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) override;

  void GetPublisherPanelInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) override;

  void SavePublisherInfo(
      const uint64_t window_id,
      ledger::PublisherInfoPtr publisher_info,
      SavePublisherInfoCallback callback) override;

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
      ledger::type::PublisherInfoPtr publisher) override;

  void OnTip(
      const std::string& publisher_key,
      const double amount,
      const bool recurring) override;

  void SetPublisherMinVisitTime(int duration_in_seconds) const override;

  void FetchBalance(FetchBalanceCallback callback) override;

  std::map<std::string, ledger::type::ExternalWalletPtr>
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
      ledger::type::ExternalWalletPtr wallet) override;

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

  void GetEventLogs(GetEventLogsCallback callback) override;

  void StopLedger(StopLedgerCallback callback);

  // Testing methods
  void SetLedgerEnvForTesting();
  void PrepareLedgerEnvForTesting();
  void StartMonthlyContributionForTest();
  void MaybeShowNotificationAddFundsForTesting(
      base::OnceCallback<void(bool)> callback);
  void CheckInsufficientFundsForTesting();
  ledger::type::TransferFeeList GetTransferFeesForTesting(
      const std::string& wallet_type);
  bool IsWalletInitialized();
  void ForTestingSetTestResponseCallback(GetTestResponseCallback callback);

 private:
  friend class ::RewardsFlagBrowserTest;

  void EnableGreaseLion(const bool enabled);

  void OnStopLedger(
      StopLedgerCallback callback,
      const ledger::type::Result result);

  void OnStopLedgerForCompleteReset(
      SuccessCallback callback,
      const ledger::type::Result result);

  void Reset();

  bool ResetOnFilesTaskRunner();

  void OnCreate();

  void OnResult(
      ledger::ResultCallback callback,
      const ledger::type::Result result);

  void OnCreateWallet(CreateWalletCallback callback,
                      ledger::type::Result result);
  void OnLedgerStateLoaded(ledger::client::OnLoadCallback callback,
                              std::pair<std::string, base::Value> data);
  void OnPublisherStateLoaded(ledger::client::OnLoadCallback callback,
                              const std::string& data);
  void OnGetRewardsParameters(
      GetRewardsParametersCallback callback,
      ledger::type::RewardsParametersPtr parameters);
  void OnFetchPromotions(
    const ledger::type::Result result,
    ledger::type::PromotionList promotions);
  void TriggerOnPromotion(
      const ledger::type::Result result,
      ledger::type::PromotionPtr promotion);
  void TriggerOnRewardsMainEnabled(bool rewards_main_enabled);
  void OnRestorePublishers(const ledger::type::Result result);
  void OnSavedState(ledger::ResultCallback callback, bool success);
  void OnLoadedState(ledger::client::OnLoadCallback callback,
                     const std::string& value);
  void OnResetState(ledger::ResultCallback callback,
                                 bool success);
  void OnTipPublisherInfoSaved(
      const ledger::type::Result result,
      ledger::type::PublisherInfoPtr info,
      const bool recurring,
      const double amount);

  void OnRecurringTip(const ledger::type::Result result);

  void TriggerOnGetCurrentBalanceReport(
      ledger::type::BalanceReportInfoPtr report);
  void MaybeShowBackupNotification(uint64_t boot_stamp);
  void MaybeShowAddFundsNotification(uint64_t reconcile_stamp);

  void OnGetOneTimeTips(
      GetRecurringTipsCallback callback,
      ledger::type::PublisherInfoList list);

  void OnInlineTipSetting(
      GetInlineTippingPlatformEnabledCallback callback,
      bool enabled);

  void OnShareURL(GetShareURLCallback callback, const std::string& url);

  void OnGetPendingContributions(
    GetPendingContributionsCallback callback,
    ledger::type::PendingContributionInfoList list);

  void OnURLLoaderComplete(network::SimpleURLLoader* loader,
                           ledger::client::LoadURLCallback callback,
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

  void OnPendingContributionRemoved(const ledger::type::Result result);

  void OnRemoveAllPendingContributions(const ledger::type::Result result);

  void OnFetchBalance(FetchBalanceCallback callback,
                      const ledger::type::Result result,
                      ledger::type::BalancePtr balance);

  void OnGetExternalWallet(
    const std::string& wallet_type,
    GetExternalWalletCallback callback,
    const ledger::type::Result result,
    ledger::type::ExternalWalletPtr wallet);

  void OnExternalWalletAuthorization(
    const std::string& wallet_type,
    ExternalWalletAuthorizationCallback callback,
    const ledger::type::Result result,
    const base::flat_map<std::string, std::string>& args);

  void OnProcessExternalWalletAuthorization(
    const std::string& wallet_type,
    const std::string& action,
    ProcessRewardsPageUrlCallback callback,
    const ledger::type::Result result,
    const std::map<std::string, std::string>& args);

  void OnDisconnectWallet(
    const std::string& wallet_type,
    const ledger::type::Result result);

  void OnSetPublisherExclude(const std::string& publisher_key,
                             const bool exclude,
                             const ledger::type::Result result);

  void OnWalletInitialized(ledger::type::Result result);

  void OnClaimPromotion(
      ClaimPromotionCallback callback,
      const ledger::type::Result result,
      const std::string& response);

  void AttestationAndroid(
      const std::string& promotion_id,
      AttestPromotionCallback callback,
      const ledger::type::Result result,
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
      const ledger::type::Result result);

  void OnRecoverWallet(const ledger::type::Result result);

  // ledger::LedgerClient
  void OnReconcileComplete(
      const ledger::type::Result result,
      ledger::type::ContributionInfoPtr contribution) override;
  void OnAttestPromotion(
      AttestPromotionCallback callback,
      const ledger::type::Result result,
      ledger::type::PromotionPtr promotion);
  void LoadLedgerState(ledger::client::OnLoadCallback callback) override;
  void LoadPublisherState(ledger::client::OnLoadCallback callback) override;
  void LoadURL(
      ledger::type::UrlRequestPtr request,
      ledger::client::LoadURLCallback callback) override;
  void SetRewardsMainEnabled(bool enabled) override;
  void SetPublisherMinVisits(int visits) const override;
  void SetPublisherAllowNonVerified(bool allow) const override;
  void SetPublisherAllowVideos(bool allow) const override;
  void OnPanelPublisherInfo(const ledger::type::Result result,
                            ledger::type::PublisherInfoPtr info,
                            uint64_t window_id) override;
  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    ledger::client::FetchIconCallback callback) override;
  void OnFetchFavIconCompleted(ledger::client::FetchIconCallback callback,
                          const std::string& favicon_key,
                          const GURL& url,
                          const SkBitmap& image);
  void OnSetOnDemandFaviconComplete(const std::string& favicon_url,
                                    ledger::client::FetchIconCallback callback,
                                    bool success);
  void OnPublisherInfo(
      GetPublisherInfoCallback callback,
      const ledger::type::Result result,
      ledger::type::PublisherInfoPtr info);
  void OnPublisherPanelInfo(
      GetPublisherInfoCallback callback,
      const ledger::type::Result result,
      ledger::type::PublisherInfoPtr info);
  void OnSavePublisherInfo(
      SavePublisherInfoCallback callback,
      const ledger::type::Result result);

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

  void PublisherListNormalized(ledger::type::PublisherInfoList list) override;

  void ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      ledger::ResultCallback callback) override;

  void SetTransferFee(
      const std::string& wallet_type,
      ledger::type::TransferFeePtr transfer_fee) override;

  ledger::type::TransferFeeList GetTransferFees(
      const std::string& wallet_type) override;

  void RemoveTransferFee(
      const std::string& wallet_type,
      const std::string& id) override;

  ledger::type::ClientInfoPtr GetClientInfo() override;

  void UnblindedTokensReady() override;

  void ReconcileStampReset() override;

  void RunDBTransaction(
      ledger::type::DBTransactionPtr transaction,
      ledger::client::RunDBTransactionCallback callback) override;

  void GetCreateScript(
      ledger::client::GetCreateScriptCallback callback) override;

  void PendingContributionSaved(const ledger::type::Result result) override;

  void OnTipPublisherSaved(
      const std::string& publisher_key,
      const double amount,
      const bool recurring,
      const ledger::type::Result result);

  void ClearAllNotifications() override;

  void WalletDisconnected(const std::string& wallet_type) override;

  void DeleteLog(ledger::ResultCallback callback) override;

  // end ledger::LedgerClient

  // Mojo Proxy methods
  void OnGetAutoContributeProperties(
      const GetAutoContributePropertiesCallback& callback,
      ledger::type::AutoContributePropertiesPtr props);
  void OnGetRewardsInternalsInfo(GetRewardsInternalsInfoCallback callback,
                                 ledger::type::RewardsInternalsInfoPtr info);

  void OnRefreshPublisher(
      RefreshPublisherCallback callback,
      const std::string& publisher_key,
      ledger::type::PublisherStatus status);
  void OnMediaInlineInfoSaved(
      SaveMediaInfoCallback callback,
      const ledger::type::Result result,
      ledger::type::PublisherInfoPtr publisher);

  void OnContributeUnverifiedPublishers(
      ledger::type::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) override;

  bool Connected() const;
  void ConnectionClosed();
  void AddPrivateObserver(RewardsServicePrivateObserver* observer) override;
  void RemovePrivateObserver(RewardsServicePrivateObserver* observer) override;

  void RecordBackendP3AStats();

  void OnRecordBackendP3AStatsRecurring(ledger::type::PublisherInfoList list);

  void OnRecordBackendP3AStatsContributions(
      const uint32_t recurring_donation_size,
      ledger::type::ContributionInfoList list);

  void OnRecordBackendP3AStatsAC(
      const int auto_contributions,
      bool ac_enabled);

  void OnGetBalanceReport(
      GetBalanceReportCallback callback,
      const ledger::type::Result result,
      ledger::type::BalanceReportInfoPtr report);

  void OnGetMonthlyReport(
      GetMonthlyReportCallback callback,
      const ledger::type::Result result,
      ledger::type::MonthlyReportInfoPtr report);

  void OnRunDBTransaction(
      ledger::client::RunDBTransactionCallback callback,
      ledger::type::DBCommandResponsePtr response);

  void OnGetAllMonthlyReportIds(
      GetAllMonthlyReportIdsCallback callback,
      const std::vector<std::string>& ids);

  void OnGetAllContributions(
      GetAllContributionsCallback callback,
      ledger::type::ContributionInfoList contributions);

  void OnGetAllPromotions(
      GetAllPromotionsCallback callback,
      base::flat_map<std::string, ledger::type::PromotionPtr> promotions);

  void OnCompleteReset(SuccessCallback callback, const bool success);

  bool DeleteLogTaskRunner();

  void OnDeleteLog(ledger::ResultCallback callback, const bool success);

  void OnGetEventLogs(
      GetEventLogsCallback callback,
      ledger::type::EventLogs logs);

#if defined(OS_ANDROID)
  ledger::type::Environment GetServerEnvironmentForAndroid();
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
  std::unique_ptr<ledger::LedgerDatabase> ledger_database_;
  std::unique_ptr<RewardsNotificationServiceImpl> notification_service_;
  base::ObserverList<RewardsServicePrivateObserver> private_observers_;
  std::unique_ptr<RewardsServiceObserver> extension_observer_;
  std::unique_ptr<RewardsServicePrivateObserver> private_observer_;

  std::unique_ptr<base::OneShotEvent> ready_;
  base::flat_set<network::SimpleURLLoader*> url_loaders_;
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
