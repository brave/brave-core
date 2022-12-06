/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/one_shot_event.h"
#include "base/sequence_checker.h"
#include "base/threading/sequence_bound.h"
#include "base/values.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/brave_rewards/browser/diagnostic_log.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "build/build_config.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/gfx/image/image.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/components/safetynet/safetynet_check.h"
#endif

#if BUILDFLAG(ENABLE_GREASELION)
#include "brave/components/greaselion/browser/greaselion_service.h"
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

class Profile;
class RewardsFlagBrowserTest;

namespace brave_rewards {

class RewardsNotificationServiceImpl;
class RewardsBrowserTest;

using GetEnvironmentCallback =
    base::OnceCallback<void(ledger::mojom::Environment)>;
using GetDebugCallback = base::OnceCallback<void(bool)>;
using GetReconcileIntervalCallback = base::OnceCallback<void(int32_t)>;
using GetGeminiRetriesCallback = base::OnceCallback<void(int32_t)>;
using GetRetryIntervalCallback = base::OnceCallback<void(int32_t)>;
using GetTestResponseCallback = base::RepeatingCallback<void(
    const std::string& url,
    int32_t method,
    int* response_status_code,
    std::string* response,
    base::flat_map<std::string, std::string>* headers)>;

using StopLedgerCallback = base::OnceCallback<void(ledger::mojom::Result)>;

class RewardsServiceImpl : public RewardsService,
                           public ledger::LedgerClient,
#if BUILDFLAG(ENABLE_GREASELION)
                           public greaselion::GreaselionService::Observer,
#endif
                           public base::SupportsWeakPtr<RewardsServiceImpl> {
 public:
#if BUILDFLAG(ENABLE_GREASELION)
  explicit RewardsServiceImpl(
      Profile* profile,
      greaselion::GreaselionService* greaselion_service);
#else
  explicit RewardsServiceImpl(Profile* profile);
#endif
  RewardsServiceImpl(const RewardsServiceImpl&) = delete;
  RewardsServiceImpl& operator=(const RewardsServiceImpl&) = delete;
  ~RewardsServiceImpl() override;

  // KeyedService:
  void Shutdown() override;

  bool IsInitialized() override;

  void Init(std::unique_ptr<RewardsServiceObserver> extension_observer,
            std::unique_ptr<RewardsNotificationServiceObserver>
                notification_observer);

  void CreateRewardsWallet(const std::string& country,
                           CreateRewardsWalletCallback callback) override;

  base::Version GetUserVersion() const override;

  std::string GetCountryCode() const override;

  void GetAvailableCountries(
      GetAvailableCountriesCallback callback) const override;

  void GetRewardsParameters(GetRewardsParametersCallback callback) override;

  void FetchPromotions(FetchPromotionsCallback callback) override;

  void ClaimPromotion(
      const std::string& promotion_id,
      ClaimPromotionCallback callback) override;
  void ClaimPromotion(
      const std::string& promotion_id,
      AttestPromotionCallback callback) override;
  void AttestPromotion(const std::string& promotion_id,
                       const std::string& solution,
                       AttestPromotionCallback callback) override;
  void GetActivityInfoList(const uint32_t start,
                           const uint32_t limit,
                           ledger::mojom::ActivityInfoFilterPtr filter,
                           GetPublisherInfoListCallback callback) override;

  void GetPublishersVisitedCount(
      base::OnceCallback<void(int)> callback) override;

  void GetExcludedList(GetPublisherInfoListCallback callback) override;

  void OnGetPublisherInfoList(
      GetPublisherInfoListCallback callback,
      std::vector<ledger::mojom::PublisherInfoPtr> list);
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
  void GetReconcileStamp(GetReconcileStampCallback callback) override;
  void GetAutoContributeEnabled(
      GetAutoContributeEnabledCallback callback) override;
  void GetPublisherMinVisitTime(
      GetPublisherMinVisitTimeCallback callback) override;
  void GetPublisherMinVisits(GetPublisherMinVisitsCallback callback) override;
  void GetPublisherAllowNonVerified(
      GetPublisherAllowNonVerifiedCallback callback) override;
  void GetPublisherAllowVideos(
      GetPublisherAllowVideosCallback callback) override;
  void RestorePublishers() override;
  void GetBalanceReport(
      const uint32_t month,
      const uint32_t year,
      GetBalanceReportCallback callback) override;
  void GetPublisherActivityFromUrl(
      uint64_t window_id,
      const std::string& url,
      const std::string& favicon_url,
      const std::string& publisher_blob) override;
  void GetAutoContributionAmount(
      GetAutoContributionAmountCallback callback) override;
  void GetPublisherBanner(const std::string& publisher_id,
                          GetPublisherBannerCallback callback) override;
  void OnPublisherBanner(GetPublisherBannerCallback callback,
                         ledger::mojom::PublisherBannerPtr banner);
  void RemoveRecurringTip(const std::string& publisher_key) override;
  void GetRecurringTips(GetRecurringTipsCallback callback) override;
  void SetPublisherExclude(
      const std::string& publisher_key,
      bool exclude) override;
  void SetExternalWalletType(const std::string& wallet_type) override;

  RewardsNotificationService* GetNotificationService() const override;
  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;

  void HandleFlags(const RewardsFlags& flags);
  void SetEnvironment(ledger::mojom::Environment environment);
  void GetEnvironment(GetEnvironmentCallback callback);
  void SetDebug(bool debug);
  void GetDebug(GetDebugCallback callback);
  void SetGeminiRetries(const int32_t retries);
  void GetGeminiRetries(GetGeminiRetriesCallback callback);
  void SetReconcileInterval(const int32_t interval);
  void GetReconcileInterval(GetReconcileIntervalCallback callback);
  void SetRetryInterval(int32_t interval);
  void GetRetryInterval(GetRetryIntervalCallback callback);

  void GetAutoContributeProperties(
      GetAutoContributePropertiesCallback callback) override;
  void GetPendingContributionsTotal(
      GetPendingContributionsTotalCallback callback) override;

  void GetOneTimeTips(GetOneTimeTipsCallback callback) override;
  void RefreshPublisher(const std::string& publisher_key,
                        RefreshPublisherCallback callback) override;

  void OnSaveRecurringTip(OnTipCallback callback, ledger::mojom::Result result);
  void SaveRecurringTip(const std::string& publisher_key,
                        double amount,
                        OnTipCallback callback) override;

  const RewardsNotificationService::RewardsNotificationsMap&
    GetAllNotifications() override;

  void SetAutoContributionAmount(const double amount) const override;

  void UpdateMediaDuration(
      const uint64_t window_id,
      const std::string& publisher_key,
      const uint64_t duration,
      const bool first_visit) override;

  void IsPublisherRegistered(const std::string& publisher_id,
                             base::OnceCallback<void(bool)> callback) override;

  void GetPublisherInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) override;

  void GetPublisherPanelInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) override;

  void SavePublisherInfo(const uint64_t window_id,
                         ledger::mojom::PublisherInfoPtr publisher_info,
                         SavePublisherInfoCallback callback) override;

  void SetInlineTippingPlatformEnabled(
      const std::string& key,
      bool enabled) override;

  void GetInlineTippingPlatformEnabled(
      const std::string& key,
      GetInlineTippingPlatformEnabledCallback callback) override;

  void GetShareURL(
      const base::flat_map<std::string, std::string>& args,
      GetShareURLCallback callback) override;

  void GetPendingContributions(
    GetPendingContributionsCallback callback) override;

  void RemovePendingContribution(const uint64_t id) override;

  void RemoveAllPendingContributions() override;

  void OnTip(const std::string& publisher_key,
             const double amount,
             const bool recurring,
             ledger::mojom::PublisherInfoPtr publisher) override;

  void OnTip(const std::string& publisher_key,
             double amount,
             bool recurring,
             OnTipCallback callback) override;

  void SetPublisherMinVisitTime(int duration_in_seconds) const override;

  bool IsAutoContributeSupported() const override;

  void FetchBalance(FetchBalanceCallback callback) override;

  std::string GetLegacyWallet() override;

  void GetExternalWallet(GetExternalWalletCallback callback) override;

  std::string GetExternalWalletType() const override;

  std::vector<std::string> GetExternalWalletProviders() const override;

  void ConnectExternalWallet(const std::string& path,
                             const std::string& query,
                             ConnectExternalWalletCallback) override;

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

  absl::optional<std::string> EncryptString(const std::string& value) override;

  absl::optional<std::string> DecryptString(const std::string& value) override;

  void GetRewardsWallet(GetRewardsWalletCallback callback) override;

  // Testing methods
  void SetLedgerEnvForTesting();
  void SetLedgerStateTargetVersionForTesting(int version);
  void PrepareLedgerEnvForTesting();
  void StartMonthlyContributionForTest();
  void MaybeShowNotificationAddFundsForTesting(
      base::OnceCallback<void(bool)> callback);
  void CheckInsufficientFundsForTesting();
  void ForTestingSetTestResponseCallback(
      const GetTestResponseCallback& callback);
  void StartProcessForTesting(base::OnceClosure callback);

 private:
  friend class ::RewardsFlagBrowserTest;
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

#if BUILDFLAG(ENABLE_GREASELION)
  void EnableGreaseLion();

  // GreaselionService::Observer:
  void OnRulesReady(greaselion::GreaselionService* greaselion_service) override;
#endif

  void InitPrefChangeRegistrar();

  void OnPreferenceChanged(const std::string& key);

  void CheckPreferences();

  void StartLedgerProcessIfNecessary();

  void OnStopLedger(StopLedgerCallback callback,
                    const ledger::mojom::Result result);

  void OnStopLedgerForCompleteReset(SuccessCallback callback,
                                    const ledger::mojom::Result result);

  void OnDiagnosticLogDeletedForCompleteReset(SuccessCallback callback,
                                              bool success);

  void Reset();

  void OnLedgerCreated();

  void OnResult(ledger::LegacyResultCallback callback,
                ledger::mojom::Result result);

  void OnLedgerStateLoaded(ledger::client::OnLoadCallback callback,
                              std::pair<std::string, base::Value> data);
  void OnPublisherStateLoaded(ledger::client::OnLoadCallback callback,
                              const std::string& data);

  void OnRestorePublishers(const ledger::mojom::Result result);

  void OnRecurringTip(const ledger::mojom::Result result);

  void MaybeShowAddFundsNotification(uint64_t reconcile_stamp);

  void OnURLLoaderComplete(SimpleURLLoaderList::iterator url_loader_it,
                           ledger::client::LoadURLCallback callback,
                           std::unique_ptr<std::string> response_body);

  void StartNotificationTimers();
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

  void OnPendingContributionRemoved(const ledger::mojom::Result result);

  void OnRemoveAllPendingContributions(const ledger::mojom::Result result);

  void OnSetPublisherExclude(const std::string& publisher_key,
                             const bool exclude,
                             const ledger::mojom::Result result);

  void OnLedgerInitialized(ledger::mojom::Result result);

  void OnClaimPromotion(ClaimPromotionCallback callback,
                        const ledger::mojom::Result result,
                        const std::string& response);

  void AttestationAndroid(const std::string& promotion_id,
                          AttestPromotionCallback callback,
                          const ledger::mojom::Result result,
                          const std::string& response);

  void OnAttestationAndroid(
      const std::string& promotion_id,
      AttestPromotionCallback callback,
      const std::string& nonce,
      const bool token_received,
      const std::string& token,
      const bool attestation_passed);

  // ledger::LedgerClient
  void OnReconcileComplete(
      const ledger::mojom::Result result,
      ledger::mojom::ContributionInfoPtr contribution) override;
  void OnAttestPromotion(AttestPromotionCallback callback,
                         const ledger::mojom::Result result,
                         ledger::mojom::PromotionPtr promotion);
  void LoadLedgerState(ledger::client::OnLoadCallback callback) override;
  void LoadPublisherState(ledger::client::OnLoadCallback callback) override;
  void LoadURL(ledger::mojom::UrlRequestPtr request,
               ledger::client::LoadURLCallback callback) override;
  void SetPublisherMinVisits(int visits) const override;
  void SetPublisherAllowNonVerified(bool allow) const override;
  void SetPublisherAllowVideos(bool allow) const override;
  void OnPanelPublisherInfo(const ledger::mojom::Result result,
                            ledger::mojom::PublisherInfoPtr info,
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

  void WriteDiagnosticLog(const std::string& file,
                          const int line,
                          const int verbose_level,
                          const std::string& message) override;

  void OnDiagnosticLogWritten(const bool success);

  void LoadDiagnosticLog(
      const int num_lines,
      LoadDiagnosticLogCallback callback) override;

  void OnDiagnosticLogLoaded(LoadDiagnosticLogCallback callback,
                             const std::string& value);

  void ClearDiagnosticLog(ClearDiagnosticLogCallback callback) override;

  void CompleteReset(SuccessCallback callback) override;

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
  void SetValueState(const std::string& name, base::Value value) override;
  base::Value GetValueState(const std::string& name) const override;
  void ClearState(const std::string& name) override;

  bool GetBooleanOption(const std::string& name) const override;
  int GetIntegerOption(const std::string& name) const override;
  double GetDoubleOption(const std::string& name) const override;
  std::string GetStringOption(const std::string& name) const override;
  int64_t GetInt64Option(const std::string& name) const override;
  uint64_t GetUint64Option(const std::string& name) const override;

  void PublisherListNormalized(
      std::vector<ledger::mojom::PublisherInfoPtr> list) override;

  void OnPublisherRegistryUpdated() override;
  void OnPublisherUpdated(const std::string& publisher_id) override;

  void ShowNotification(const std::string& type,
                        const std::vector<std::string>& args,
                        ledger::LegacyResultCallback callback) override;

  ledger::mojom::ClientInfoPtr GetClientInfo() override;

  void UnblindedTokensReady() override;

  void ReconcileStampReset() override;

  void RunDBTransaction(
      ledger::mojom::DBTransactionPtr transaction,
      ledger::client::RunDBTransactionCallback callback) override;

  void GetCreateScript(
      ledger::client::GetCreateScriptCallback callback) override;

  void PendingContributionSaved(const ledger::mojom::Result result) override;

  void OnTipPublisherSaved(const std::string& publisher_key,
                           const double amount,
                           const bool recurring,
                           const ledger::mojom::Result result);

  void ClearAllNotifications() override;

  void ExternalWalletConnected() const override;

  void ExternalWalletLoggedOut() const override;

  void ExternalWalletReconnected() const override;

  void DeleteLog(ledger::LegacyResultCallback callback) override;

  // end ledger::LedgerClient

  void OnRefreshPublisher(RefreshPublisherCallback callback,
                          const std::string& publisher_key,
                          ledger::mojom::PublisherStatus status);

  void OnContributeUnverifiedPublishers(
      ledger::mojom::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) override;

  void OnGetRewardsWalletForP3A(ledger::mojom::RewardsWalletPtr wallet);

  bool Connected() const;
  void ConnectionClosed();

  void RecordBackendP3AStats();

  void OnRecordBackendP3AStatsRecurring(
      std::vector<ledger::mojom::PublisherInfoPtr> list);

  void OnRecordBackendP3AStatsContributions(
      const uint32_t recurring_donation_size,
      std::vector<ledger::mojom::ContributionInfoPtr> list);

  void OnRecordBackendP3AStatsAC(
      const int auto_contributions,
      bool ac_enabled);

  void OnGetBalanceReport(GetBalanceReportCallback callback,
                          const ledger::mojom::Result result,
                          ledger::mojom::BalanceReportInfoPtr report);

  void OnGetMonthlyReport(GetMonthlyReportCallback callback,
                          const ledger::mojom::Result result,
                          ledger::mojom::MonthlyReportInfoPtr report);

  void OnRunDBTransaction(ledger::client::RunDBTransactionCallback callback,
                          ledger::mojom::DBCommandResponsePtr response);

  void OnGetAllPromotions(
      GetAllPromotionsCallback callback,
      base::flat_map<std::string, ledger::mojom::PromotionPtr> promotions);

  void OnFilesDeletedForCompleteReset(SuccessCallback callback,
                                      const bool success);

  void OnDiagnosticLogDeleted(ledger::LegacyResultCallback callback,
                              bool success);

  bool IsBitFlyerRegion() const;

  bool IsValidWalletType(const std::string& wallet_type) const;

#if BUILDFLAG(IS_ANDROID)
  ledger::mojom::Environment GetServerEnvironmentForAndroid();
  safetynet_check::SafetyNetCheckRunner safetynet_check_runner_;
#endif

  raw_ptr<Profile> profile_ = nullptr;  // NOT OWNED
#if BUILDFLAG(ENABLE_GREASELION)
  raw_ptr<greaselion::GreaselionService> greaselion_service_ =
      nullptr;  // NOT OWNED
#endif
  mojo::AssociatedReceiver<bat_ledger::mojom::BatLedgerClient>
      bat_ledger_client_receiver_;
  mojo::AssociatedRemote<bat_ledger::mojom::BatLedger> bat_ledger_;
  mojo::Remote<bat_ledger::mojom::BatLedgerService> bat_ledger_service_;
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;

  const base::FilePath ledger_state_path_;
  const base::FilePath publisher_state_path_;
  const base::FilePath publisher_info_db_path_;
  const base::FilePath publisher_list_path_;

  std::unique_ptr<DiagnosticLog> diagnostic_log_;
  base::SequenceBound<ledger::LedgerDatabase> ledger_database_;
  std::unique_ptr<RewardsNotificationServiceImpl> notification_service_;
  std::unique_ptr<RewardsServiceObserver> extension_observer_;

  std::unique_ptr<base::OneShotEvent> ready_;
  SimpleURLLoaderList url_loaders_;
  std::map<std::string, BitmapFetcherService::RequestId>
      current_media_fetchers_;
  std::unique_ptr<base::OneShotTimer> notification_startup_timer_;
  std::unique_ptr<base::RepeatingTimer> notification_periodic_timer_;
  PrefChangeRegistrar profile_pref_change_registrar_;

  uint32_t next_timer_id_;
  int32_t country_id_ = 0;
  bool reset_states_;
  bool ledger_for_testing_ = false;
  int ledger_state_target_version_for_testing_ = -1;
  bool resetting_rewards_ = false;
  int persist_log_level_ = 0;

  GetTestResponseCallback test_response_callback_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_
