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
#include "base/values.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_client.h"
#include "brave/components/brave_rewards/browser/diagnostic_log.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_private_observer.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/gfx/image/image.h"

#if defined(OS_ANDROID)
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
    base::OnceCallback<void(ledger::type::Environment)>;
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

using ExternalWalletAuthorizationCallback =
    base::OnceCallback<void(
        const ledger::type::Result,
        const base::flat_map<std::string, std::string>&)>;

using StopLedgerCallback = base::OnceCallback<void(ledger::type::Result)>;

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

  void Init(
      std::unique_ptr<RewardsServiceObserver> extension_observer,
      std::unique_ptr<RewardsServicePrivateObserver> private_observer,
      std::unique_ptr<RewardsNotificationServiceObserver>
          notification_observer);
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
  void RecoverWallet(const std::string& passPhrase) override;
  void GetActivityInfoList(const uint32_t start,
                           const uint32_t limit,
                           ledger::type::ActivityInfoFilterPtr filter,
                           GetPublisherInfoListCallback callback) override;

  void GetExcludedList(GetPublisherInfoListCallback callback) override;

  void OnGetPublisherInfoList(GetPublisherInfoListCallback callback,
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
                         ledger::type::PublisherBannerPtr banner);
  void RemoveRecurringTip(const std::string& publisher_key) override;
  void OnGetRecurringTips(
      GetRecurringTipsCallback callback,
      ledger::type::PublisherInfoList list);
  void GetRecurringTips(GetRecurringTipsCallback callback) override;
  void SetPublisherExclude(
      const std::string& publisher_key,
      bool exclude) override;
  void SetExternalWalletType(const std::string& wallet_type) override;

  RewardsNotificationService* GetNotificationService() const override;
  void SetBackupCompleted() override;
  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;

  void HandleFlags(const std::string& options);
  void SetEnvironment(ledger::type::Environment environment);
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
  void RefreshPublisher(
      const std::string& publisher_key,
      RefreshPublisherCallback callback) override;
  void OnAdsEnabled(bool ads_enabled) override;

  void OnSaveRecurringTip(OnTipCallback callback, ledger::type::Result result);
  void SaveRecurringTip(const std::string& publisher_key,
                        double amount,
                        OnTipCallback callback) override;

  const RewardsNotificationService::RewardsNotificationsMap&
    GetAllNotifications() override;

  void SetAutoContributionAmount(const double amount) const override;

  void SaveInlineMediaInfo(
      const std::string& media_type,
      const base::flat_map<std::string, std::string>& args,
      SaveMediaInfoCallback callback) override;

  void UpdateMediaDuration(
      const uint64_t window_id,
      const std::string& publisher_key,
      const uint64_t duration,
      const bool first_visit) override;

  void GetPublisherInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) override;

  void GetPublisherPanelInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) override;

  void SavePublisherInfo(
      const uint64_t window_id,
      ledger::type::PublisherInfoPtr publisher_info,
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

  void OnTip(
      const std::string& publisher_key,
      const double amount,
      const bool recurring,
      ledger::type::PublisherInfoPtr publisher) override;

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

  const std::vector<std::string> GetExternalWalletProviders() const override;

  void ExternalWalletAuthorization(
      const std::string& wallet_type,
      const base::flat_map<std::string, std::string>& args,
      ExternalWalletAuthorizationCallback callback);

  void ProcessRewardsPageUrl(
      const std::string& path,
      const std::string& query,
      ProcessRewardsPageUrlCallback callback) override;

  void RequestAdsEnabledPopupClosed(bool ads_enabled) override;

  void DisconnectWallet() override;

  void GetAnonWalletStatus(GetAnonWalletStatusCallback callback) override;

  void SetAutoContributeEnabled(bool enabled) override;

  bool ShouldShowOnboarding() const override;

  void EnableRewards() override;

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

  void GetBraveWallet(GetBraveWalletCallback callback) override;

  void StartProcess(base::OnceClosure callback) override;

  void GetWalletPassphrase(GetWalletPassphraseCallback callback) override;

  void SetAdsEnabled(const bool is_enabled) override;

  bool IsRewardsEnabled() const override;

  // Testing methods
  void SetLedgerEnvForTesting();
  void PrepareLedgerEnvForTesting();
  void StartMonthlyContributionForTest();
  void MaybeShowNotificationAddFundsForTesting(
      base::OnceCallback<void(bool)> callback);
  void CheckInsufficientFundsForTesting();
  void ForTestingSetTestResponseCallback(
      const GetTestResponseCallback& callback);

 private:
  friend class ::RewardsFlagBrowserTest;
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

#if BUILDFLAG(ENABLE_GREASELION)
  void EnableGreaseLion();

  // GreaselionService::Observer:
  void OnRulesReady(greaselion::GreaselionService* greaselion_service) override;
#endif

  void OnConnectionClosed(const ledger::type::Result result);

  void InitPrefChangeRegistrar();

  void OnPreferenceChanged(const std::string& key);

  void CheckPreferences();

  void StartLedgerProcessIfNecessary();

  void OnStopLedger(
      StopLedgerCallback callback,
      const ledger::type::Result result);

  void OnStopLedgerForCompleteReset(
      SuccessCallback callback,
      const ledger::type::Result result);

  void OnDiagnosticLogDeletedForCompleteReset(SuccessCallback callback,
                                              bool success);

  void Reset();

  void OnLedgerCreated();

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

  void WalletBackupNotification(const uint64_t boot_stamp,
                                const ledger::type::Result result,
                                ledger::type::ExternalWalletPtr wallet);

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

  void OnPendingContributionRemoved(const ledger::type::Result result);

  void OnRemoveAllPendingContributions(const ledger::type::Result result);

  void OnFetchBalance(FetchBalanceCallback callback,
                      const ledger::type::Result result,
                      ledger::type::BalancePtr balance);

  void OnGetExternalWallet(GetExternalWalletCallback callback,
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
    const base::flat_map<std::string, std::string>& args);

  void OnDisconnectWallet(
    const std::string& wallet_type,
    const ledger::type::Result result);

  void OnSetPublisherExclude(const std::string& publisher_key,
                             const bool exclude,
                             const ledger::type::Result result);

  void OnLedgerInitialized(ledger::type::Result result);

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

  void OnStartProcessForSetAdsEnabled();

  void OnWalletCreatedForSetAdsEnabled(const ledger::type::Result result);

  void OnStartProcessForEnableRewards();

  void OnFetchBalanceForEnableRewards(ledger::type::Result result,
                                      ledger::type::BalancePtr balance);

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
  void OnStartProcessForGetPublisherInfo(const std::string& publisher_key,
                                         GetPublisherInfoCallback callback);
  void OnPublisherPanelInfo(
      GetPublisherInfoCallback callback,
      const ledger::type::Result result,
      ledger::type::PublisherInfoPtr info);
  void OnStartProcessForSavePublisherInfo(
      uint64_t window_id,
      ledger::type::PublisherInfoPtr publisher_info,
      SavePublisherInfoCallback callback);
  void OnSavePublisherInfo(
      SavePublisherInfoCallback callback,
      const ledger::type::Result result);

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

  void OnDiagnosticLogCleared(ClearDiagnosticLogCallback callback,
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
      GetAutoContributePropertiesCallback callback,
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

  void OnGetBraveWalletForP3A(ledger::type::BraveWalletPtr wallet);

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

  void OnFilesDeletedForCompleteReset(SuccessCallback callback,
                                      const bool success);

  void OnStartProcessForCompleteReset(SuccessCallback callback, bool success);

  void OnDiagnosticLogDeleted(ledger::ResultCallback callback,
                              const bool success);

  void OnGetEventLogs(
      GetEventLogsCallback callback,
      ledger::type::EventLogs logs);

  void OnGetBraveWallet(
      GetBraveWalletCallback callback,
      ledger::type::BraveWalletPtr wallet);

  bool IsBitFlyerRegion() const;

  bool IsValidWalletType(const std::string& wallet_type) const;

#if defined(OS_ANDROID)
  ledger::type::Environment GetServerEnvironmentForAndroid();
  void GrantAttestationResult(
      const std::string& promotion_id, bool result,
      const std::string& result_string);
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
  std::unique_ptr<ledger::LedgerDatabase> ledger_database_;
  std::unique_ptr<RewardsNotificationServiceImpl> notification_service_;
  base::ObserverList<RewardsServicePrivateObserver> private_observers_;
  std::unique_ptr<RewardsServiceObserver> extension_observer_;
  std::unique_ptr<RewardsServicePrivateObserver> private_observer_;

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
  bool resetting_rewards_ = false;
  int persist_log_level_ = 0;

  GetTestResponseCallback test_response_callback_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_
