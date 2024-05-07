/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/one_shot_event.h"
#include "base/sequence_checker.h"
#include "base/threading/sequence_bound.h"
#include "base/time/time.h"
#include "base/timer/wall_clock_timer.h"
#include "base/values.h"
#include "brave/components/brave_rewards/browser/diagnostic_log.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/mojom/rewards_engine.mojom.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/services/bat_rewards/public/interfaces/rewards_engine_factory.mojom.h"
#include "build/build_config.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/gfx/image/image.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/components/safetynet/safetynet_check.h"
#endif

namespace base {
class OneShotTimer;
class SequencedTaskRunner;
}  // namespace base

namespace favicon {
class FaviconService;
}  // namespace favicon
namespace network {
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {
class BraveWalletService;
}

namespace brave_rewards {

class RewardsFlagBrowserTest;

namespace internal {
class RewardsDatabase;
}  // namespace internal

class RewardsNotificationServiceImpl;
class RewardsBrowserTest;

using GetDebugCallback = base::OnceCallback<void(bool)>;
using GetReconcileIntervalCallback = base::OnceCallback<void(int32_t)>;
using GetRetryIntervalCallback = base::OnceCallback<void(int32_t)>;
using GetTestResponseCallback = base::RepeatingCallback<void(
    const std::string& url,
    int32_t method,
    int* response_status_code,
    std::string* response,
    base::flat_map<std::string, std::string>* headers)>;

using StopEngineCallback = base::OnceCallback<void(mojom::Result)>;
using RequestImageCallback = base::RepeatingCallback<int(
    const GURL& url,
    base::OnceCallback<void(const SkBitmap& bitmap)>,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)>;
using CancelImageRequestCallback = base::RepeatingCallback<void(int)>;
class RewardsServiceImpl final : public RewardsService,
                                 public mojom::RewardsEngineClient {
 public:
  RewardsServiceImpl(PrefService* prefs,
                     const base::FilePath& profile_path,
                     favicon::FaviconService* favicon_service,
                     RequestImageCallback request_image_callback,
                     CancelImageRequestCallback cancel_image_request_callback,
                     content::StoragePartition* storage_partition,
                     brave_wallet::BraveWalletService* brave_wallet_service);

  RewardsServiceImpl(const RewardsServiceImpl&) = delete;
  RewardsServiceImpl& operator=(const RewardsServiceImpl&) = delete;
  ~RewardsServiceImpl() override;

  static std::string UrlMethodToRequestType(mojom::UrlMethod method);

  // KeyedService:
  void Shutdown() override;

  bool IsInitialized() override;

  void Init(std::unique_ptr<RewardsServiceObserver> extension_observer,
            std::unique_ptr<RewardsNotificationServiceObserver>
                notification_observer);

  void CreateRewardsWallet(const std::string& country,
                           CreateRewardsWalletCallback callback) override;

  void GetUserType(base::OnceCallback<void(mojom::UserType)> callback) override;

  bool IsTermsOfServiceUpdateRequired() override;

  void AcceptTermsOfServiceUpdate() override;

  std::string GetCountryCode() const override;

  void GetAvailableCountries(
      GetAvailableCountriesCallback callback) const override;

  void GetRewardsParameters(GetRewardsParametersCallback callback) override;

  void FetchUICards(FetchUICardsCallback callback) override;

  void GetActivityInfoList(const uint32_t start,
                           const uint32_t limit,
                           mojom::ActivityInfoFilterPtr filter,
                           GetPublisherInfoListCallback callback) override;

  void GetPublishersVisitedCount(
      base::OnceCallback<void(int)> callback) override;

  void GetExcludedList(GetPublisherInfoListCallback callback) override;

  void OnGetPublisherInfoList(GetPublisherInfoListCallback callback,
                              std::vector<mojom::PublisherInfoPtr> list);
  void OnLoad(mojom::VisitDataPtr visit_data) override;
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
  void GetReconcileStamp(GetReconcileStampCallback callback) override;
  void GetPublisherMinVisitTime(
      GetPublisherMinVisitTimeCallback callback) override;
  void GetPublisherMinVisits(GetPublisherMinVisitsCallback callback) override;
  void RestorePublishers() override;
  void GetBalanceReport(
      const uint32_t month,
      const uint32_t year,
      GetBalanceReportCallback callback) override;
  void GetPublisherActivityFromVisitData(
      mojom::VisitDataPtr visit_data) override;
  void GetPublisherActivityFromUrl(uint64_t tab_id,
                                   const std::string& url,
                                   const std::string& favicon_url,
                                   const std::string& publisher_blob) override;
  void GetPublisherBanner(const std::string& publisher_id,
                          GetPublisherBannerCallback callback) override;
  void OnPublisherBanner(GetPublisherBannerCallback callback,
                         mojom::PublisherBannerPtr banner);
  void RemoveRecurringTip(const std::string& publisher_key) override;
  void GetRecurringTips(GetRecurringTipsCallback callback) override;
  void SetPublisherExclude(
      const std::string& publisher_key,
      bool exclude) override;

  RewardsNotificationService* GetNotificationService() const override;
  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;
  void GetEnvironment(GetEnvironmentCallback callback) override;

  p3a::ConversionMonitor* GetP3AConversionMonitor() override;
  void OnRewardsPageShown() override;

  mojom::RewardsEngineOptionsPtr HandleFlags(const RewardsFlags& flags);

  void GetOneTimeTips(GetOneTimeTipsCallback callback) override;
  void RefreshPublisher(const std::string& publisher_key,
                        RefreshPublisherCallback callback) override;

  void OnSaveRecurringTip(OnTipCallback callback, mojom::Result result);

  void SaveRecurringTip(const std::string& publisher_key,
                        double amount,
                        OnTipCallback callback) override;

  void SendContribution(const std::string& publisher_id,
                        double amount,
                        bool set_monthly,
                        base::OnceCallback<void(bool)> callback) override;

  const RewardsNotificationService::RewardsNotificationsMap&
    GetAllNotifications() override;

  void IsPublisherRegistered(const std::string& publisher_id,
                             base::OnceCallback<void(bool)> callback) override;

  void GetPublisherInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) override;

  void GetPublisherPanelInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) override;

  void SavePublisherInfo(const uint64_t window_id,
                         mojom::PublisherInfoPtr publisher_info,
                         SavePublisherInfoCallback callback) override;

  void GetShareURL(
      const base::flat_map<std::string, std::string>& args,
      GetShareURLCallback callback) override;

  void OnTip(const std::string& publisher_key,
             double amount,
             bool recurring,
             OnTipCallback callback) override;

  void SetPublisherMinVisitTime(int duration_in_seconds) const override;

  void FetchBalance(FetchBalanceCallback callback) override;

  void GetExternalWallet(GetExternalWalletCallback callback) override;

  std::string GetExternalWalletType() const override;

  std::vector<std::string> GetExternalWalletProviders() const override;

  void BeginExternalWalletLogin(
      const std::string& wallet_type,
      BeginExternalWalletLoginCallback callback) override;

  void ConnectExternalWallet(const std::string& path,
                             const std::string& query,
                             ConnectExternalWalletCallback) override;

  void ConnectExternalWallet(
      const std::string& provider,
      const base::flat_map<std::string, std::string>& args,
      ConnectExternalWalletCallback callback) override;

  void GetAllContributions(GetAllContributionsCallback callback) override;

  void GetEventLogs(GetEventLogsCallback callback) override;

  void StopEngine(StopEngineCallback callback);

  void EncryptString(const std::string& value,
                     EncryptStringCallback callback) override;

  void DecryptString(const std::string& value,
                     DecryptStringCallback callback) override;

  void GetRewardsWallet(GetRewardsWalletCallback callback) override;

  base::WeakPtr<RewardsServiceImpl> AsWeakPtr();

  // Testing methods
  void SetEngineEnvForTesting();
  void PrepareEngineEnvForTesting(mojom::RewardsEngineOptions& options);
  void StartContributionsForTesting();
  void ForTestingSetTestResponseCallback(
      const GetTestResponseCallback& callback);
  void StartProcessForTesting(base::OnceClosure callback);

 private:
  friend class RewardsFlagBrowserTest;
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;

  void InitPrefChangeRegistrar();

  void OnPreferenceChanged(const std::string& key);

  void CheckPreferences();

  void StartEngineProcessIfNecessary();

  void OnStopEngine(StopEngineCallback callback, const mojom::Result result);

  void OnStopEngineForCompleteReset(SuccessCallback callback,
                                    const mojom::Result result);

  void OnDiagnosticLogDeletedForCompleteReset(SuccessCallback callback,
                                              bool success);

  void Reset();

  void OnEngineCreated();

  void OnRestorePublishers(const mojom::Result result);

  void OnRecurringTip(const mojom::Result result);

  void OnContributionSent(bool set_monthly,
                          base::OnceCallback<void(bool)> callback,
                          bool success);

  void OnURLLoaderComplete(SimpleURLLoaderList::iterator url_loader_it,
                           LoadURLCallback callback,
                           std::unique_ptr<std::string> response_body);

  void OnGetSPLTokenAccountBalance(
      GetSPLTokenAccountBalanceCallback callback,
      const std::string& amount,
      uint8_t decimals,
      const std::string& amount_string,
      brave_wallet::mojom::SolanaProviderError error,
      const std::string& error_message);

  void ShowNotificationTipsPaid();

  void OnSetPublisherExclude(const std::string& publisher_key,
                             const bool exclude,
                             const mojom::Result result);

  void OnEngineInitialized(mojom::Result result);

  void OnExternalWalletLoginStarted(BeginExternalWalletLoginCallback callback,
                                    mojom::ExternalWalletLoginParamsPtr params);

  // mojom::RewardsEngineClient
  void OnReconcileComplete(mojom::Result result,
                           mojom::ContributionInfoPtr contribution) override;
  void LoadURL(mojom::UrlRequestPtr request, LoadURLCallback callback) override;
  void GetSPLTokenAccountBalance(
      const std::string& solana_address,
      const std::string& token_mint_address,
      GetSPLTokenAccountBalanceCallback callback) override;
  void SetPublisherMinVisits(int visits) const override;
  void OnPanelPublisherInfo(mojom::Result result,
                            mojom::PublisherInfoPtr info,
                            uint64_t tab_id) override;
  void FetchFavIcon(const std::string& url,
                    const std::string& favicon_key,
                    FetchFavIconCallback callback) override;
  void OnFetchFavIconCompleted(FetchFavIconCallback callback,
                               const std::string& favicon_key,
                               const GURL& url,
                               const SkBitmap& image);
  void OnSetOnDemandFaviconComplete(const std::string& favicon_url,
                                    FetchFavIconCallback callback,
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

  void Log(const std::string& file,
           int32_t line,
           int32_t verbose_level,
           const std::string& message) override;

  void GetUserPreferenceValue(const std::string& path,
                              GetUserPreferenceValueCallback callback) override;

  void SetUserPreferenceValue(const std::string& path,
                              base::Value value,
                              SetUserPreferenceValueCallback callback) override;

  void ClearUserPreferenceValue(
      const std::string& path,
      ClearUserPreferenceValueCallback callback) override;

  void PublisherListNormalized(
      std::vector<mojom::PublisherInfoPtr> list) override;

  void OnPublisherRegistryUpdated() override;
  void OnPublisherUpdated(const std::string& publisher_id) override;

  void ShowNotification(const std::string& type,
                        const std::vector<std::string>& args,
                        ShowNotificationCallback callback) override;

  void ReconcileStampReset() override;

  void RunDBTransaction(mojom::DBTransactionPtr transaction,
                        RunDBTransactionCallback callback) override;

  void ClearAllNotifications() override;

  void ExternalWalletConnected() override;

  void ExternalWalletLoggedOut() override;

  void ExternalWalletReconnected() override;

  void ExternalWalletDisconnected() override;

  void DeleteLog(DeleteLogCallback callback) override;

  // end mojom::RewardsEngineClient

  void OnRefreshPublisher(RefreshPublisherCallback callback,
                          const std::string& publisher_key,
                          mojom::PublisherStatus status);

  bool Connected() const;
  void ConnectionClosed();

  void RecordBackendP3AStats(bool delay_report = false);
  void OnP3ADailyTimer();

  void OnRecordBackendP3AExternalWallet(bool delay_report,
                                        bool search_result_optin_changed,
                                        mojom::ExternalWalletPtr wallet);
  void GetAllContributionsForP3A();
  void OnRecordBackendP3AStatsContributions(
      std::vector<mojom::ContributionInfoPtr> list);
  void OnRecordBackendP3AStatsRecurringTips(
      std::vector<mojom::PublisherInfoPtr> list);

  void OnGetBalanceReport(GetBalanceReportCallback callback,
                          const mojom::Result result,
                          mojom::BalanceReportInfoPtr report);

  void OnRunDBTransaction(RunDBTransactionCallback callback,
                          mojom::DBCommandResponsePtr response);

  void OnFilesDeletedForCompleteReset(SuccessCallback callback,
                                      const bool success);

  void OnDiagnosticLogDeleted(DeleteLogCallback callback, bool success);

  bool IsBitFlyerCountry() const;

  bool IsValidWalletType(const std::string& wallet_type) const;

  static void OnGetRewardsParameters(GetRewardsParametersCallback,
                                     mojom::RewardsParametersPtr);

  mojom::Environment GetDefaultServerEnvironment();

#if BUILDFLAG(IS_ANDROID)
  mojom::Environment GetDefaultServerEnvironmentForAndroid();
  safetynet_check::SafetyNetCheckRunner safetynet_check_runner_;
#endif
  raw_ptr<PrefService> prefs_;                            // NOT OWNED
  raw_ptr<favicon::FaviconService, DanglingUntriaged> favicon_service_;
  const RequestImageCallback request_image_callback_;
  const CancelImageRequestCallback cancel_image_request_callback_;
  raw_ptr<content::StoragePartition> storage_partition_;  // NOT OWNED
  raw_ptr<brave_wallet::BraveWalletService> brave_wallet_service_ = nullptr;
  mojo::AssociatedReceiver<mojom::RewardsEngineClient> receiver_;
  mojo::AssociatedRemote<mojom::RewardsEngine> engine_;
  mojo::Remote<mojom::RewardsEngineFactory> engine_factory_;
  const scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  const scoped_refptr<base::SequencedTaskRunner> json_sanitizer_task_runner_;

  const base::FilePath legacy_state_path_;
  const base::FilePath publisher_state_path_;
  const base::FilePath publisher_info_db_path_;
  const base::FilePath publisher_list_path_;

  std::unique_ptr<DiagnosticLog> diagnostic_log_;
  base::SequenceBound<internal::RewardsDatabase> rewards_database_;
  std::unique_ptr<RewardsNotificationServiceImpl> notification_service_;
  std::unique_ptr<RewardsServiceObserver> extension_observer_;

  std::unique_ptr<base::OneShotEvent> ready_;
  SimpleURLLoaderList url_loaders_;
  std::map<std::string, int> current_media_fetchers_;
  PrefChangeRegistrar profile_pref_change_registrar_;

  bool engine_for_testing_ = false;
  bool resetting_rewards_ = false;
  int persist_log_level_ = 0;
  base::WallClockTimer p3a_daily_timer_;
  base::OneShotTimer p3a_tip_report_timer_;

  GetTestResponseCallback test_response_callback_;

  p3a::ConversionMonitor conversion_monitor_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<RewardsServiceImpl> weak_ptr_factory_{this};
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_IMPL_H_
