/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_IMPL_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/ledger.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"

namespace bat_ledger {

class BatLedgerClientMojoBridge;

class BatLedgerImpl :
    public mojom::BatLedger,
    public base::SupportsWeakPtr<BatLedgerImpl> {
 public:
  explicit BatLedgerImpl(
      mojo::PendingAssociatedRemote<mojom::BatLedgerClient> client_info);
  ~BatLedgerImpl() override;

  BatLedgerImpl(const BatLedgerImpl&) = delete;
  BatLedgerImpl& operator=(const BatLedgerImpl&) = delete;

  // bat_ledger::mojom::BatLedger
  void Initialize(
    const bool execute_create_script,
    InitializeCallback callback) override;
  void CreateRewardsWallet(CreateRewardsWalletCallback callback) override;
  void GetRewardsParameters(GetRewardsParametersCallback callback) override;

  void GetAutoContributeProperties(
      GetAutoContributePropertiesCallback callback) override;
  void GetPublisherMinVisitTime(
      GetPublisherMinVisitTimeCallback callback) override;
  void GetPublisherMinVisits(
      GetPublisherMinVisitsCallback callback) override;
  void GetPublisherAllowNonVerified(
      GetPublisherAllowNonVerifiedCallback callback) override;
  void GetPublisherAllowVideos(
      GetPublisherAllowVideosCallback callback) override;
  void GetAutoContributeEnabled(
      GetAutoContributeEnabledCallback callback) override;
  void GetReconcileStamp(GetReconcileStampCallback callback) override;

  void OnLoad(ledger::mojom::VisitDataPtr visit_data,
              uint64_t current_time) override;
  void OnUnload(uint32_t tab_id, uint64_t current_time) override;
  void OnShow(uint32_t tab_id, uint64_t current_time) override;
  void OnHide(uint32_t tab_id, uint64_t current_time) override;
  void OnForeground(uint32_t tab_id, uint64_t current_time) override;
  void OnBackground(uint32_t tab_id, uint64_t current_time) override;

  void OnPostData(const std::string& url,
                  const std::string& first_party_url,
                  const std::string& referrer,
                  const std::string& post_data,
                  ledger::mojom::VisitDataPtr visit_data) override;
  void OnXHRLoad(uint32_t tab_id,
                 const std::string& url,
                 const base::flat_map<std::string, std::string>& parts,
                 const std::string& first_party_url,
                 const std::string& referrer,
                 ledger::mojom::VisitDataPtr visit_data) override;

  void SetPublisherExclude(const std::string& publisher_key,
                           ledger::mojom::PublisherExclude exclude,
                           SetPublisherExcludeCallback callback) override;
  void RestorePublishers(RestorePublishersCallback callback) override;

  void FetchPromotions(FetchPromotionsCallback callback) override;
  void ClaimPromotion(
      const std::string& promotion_id,
      const std::string& payload,
      ClaimPromotionCallback callback) override;
  void AttestPromotion(
      const std::string& promotion_id,
      const std::string& solution,
      AttestPromotionCallback callback) override;
  void RecoverWallet(
      const std::string& pass_phrase,
      RecoverWalletCallback callback) override;

  void SetPublisherMinVisitTime(int duration_in_seconds) override;
  void SetPublisherMinVisits(int visits) override;
  void SetPublisherAllowNonVerified(bool allow) override;
  void SetPublisherAllowVideos(bool allow) override;
  void SetAutoContributionAmount(double amount) override;
  void SetAutoContributeEnabled(bool enabled) override;

  void GetBalanceReport(ledger::mojom::ActivityMonth month,
                        int32_t year,
                        GetBalanceReportCallback callback) override;

  void GetPublisherActivityFromUrl(uint64_t window_id,
                                   ledger::mojom::VisitDataPtr visit_data,
                                   const std::string& publisher_blob) override;

  void GetAutoContributionAmount(
      GetAutoContributionAmountCallback callback) override;
  void GetPublisherBanner(const std::string& publisher_id,
      GetPublisherBannerCallback callback) override;

  void OneTimeTip(
      const std::string& publisher_key,
      const double amount,
      OneTimeTipCallback callback) override;

  void RemoveRecurringTip(
      const std::string& publisher_key,
      RemoveRecurringTipCallback callback) override;
  void GetCreationStamp(GetCreationStampCallback callback) override;
  void HasSufficientBalanceToReconcile(
      HasSufficientBalanceToReconcileCallback callback) override;

  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;
  void RefreshPublisher(
      const std::string& publisher_key,
      RefreshPublisherCallback callback) override;
  void StartMonthlyContribution() override;

  void SaveRecurringTip(ledger::mojom::RecurringTipPtr info,
                        SaveRecurringTipCallback callback) override;

  void GetRecurringTips(GetRecurringTipsCallback callback) override;

  void GetOneTimeTips(GetOneTimeTipsCallback callback) override;

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           ledger::mojom::ActivityInfoFilterPtr filter,
                           GetActivityInfoListCallback callback) override;

  void GetExcludedList(GetExcludedListCallback callback) override;

  void UpdateMediaDuration(
      const uint64_t window_id,
      const std::string& publisher_key,
      const uint64_t duration,
      const bool first_visit) override;

  void IsPublisherRegistered(const std::string& publisher_id,
                             IsPublisherRegisteredCallback callback) override;

  void GetPublisherInfo(
      const std::string& publisher_key,
      GetPublisherInfoCallback callback) override;

  void GetPublisherPanelInfo(
      const std::string& publisher_key,
      GetPublisherPanelInfoCallback callback) override;

  void SavePublisherInfo(const uint64_t window_id,
                         ledger::mojom::PublisherInfoPtr publisher_info,
                         SavePublisherInfoCallback callback) override;

  void SetInlineTippingPlatformEnabled(
      const ledger::mojom::InlineTipsPlatforms platform,
      bool enabled) override;

  void GetInlineTippingPlatformEnabled(
      const ledger::mojom::InlineTipsPlatforms platform,
      GetInlineTippingPlatformEnabledCallback callback) override;

  void GetShareURL(
    const base::flat_map<std::string, std::string>& args,
    GetShareURLCallback callback) override;

  void GetPendingContributions(
    GetPendingContributionsCallback callback) override;

  void RemovePendingContribution(
      const uint64_t id,
      RemovePendingContributionCallback callback) override;

  void RemoveAllPendingContributions(
    RemovePendingContributionCallback callback) override;

  void GetPendingContributionsTotal(
    GetPendingContributionsTotalCallback callback) override;

  void FetchBalance(FetchBalanceCallback callback) override;

  void GetExternalWallet(const std::string& wallet_type,
                         GetExternalWalletCallback) override;

  void ExternalWalletAuthorization(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ExternalWalletAuthorizationCallback callback) override;

  void DisconnectWallet(
    const std::string& wallet_type,
    DisconnectWalletCallback callback) override;

  void GetTransactionReport(const ledger::mojom::ActivityMonth month,
                            const int year,
                            GetTransactionReportCallback callback) override;

  void GetContributionReport(const ledger::mojom::ActivityMonth month,
                             const int year,
                             GetContributionReportCallback callback) override;

  void GetAllContributions(GetAllContributionsCallback callback) override;

  void SavePublisherInfoForTip(
      ledger::mojom::PublisherInfoPtr info,
      SavePublisherInfoForTipCallback callback) override;

  void GetMonthlyReport(const ledger::mojom::ActivityMonth month,
                        const int year,
                        GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(
      GetAllMonthlyReportIdsCallback callback) override;

  void GetAllPromotions(GetAllPromotionsCallback callback) override;

  void Shutdown(ShutdownCallback callback) override;

  void GetEventLogs(GetEventLogsCallback callback) override;

  void GetRewardsWallet(GetRewardsWalletCallback callback) override;

  void GetRewardsWalletPassphrase(
      GetRewardsWalletPassphraseCallback callback) override;

 private:
  // workaround to pass base::OnceCallback into std::bind
  template <typename Callback>
  class CallbackHolder {
   public:
    CallbackHolder(base::WeakPtr<BatLedgerImpl> client, Callback callback)
        : client_(client), callback_(std::move(callback)) {}
    ~CallbackHolder() = default;
    bool is_valid() { return !!client_.get(); }
    Callback& get() { return callback_; }

   private:
    base::WeakPtr<BatLedgerImpl> client_;
    Callback callback_;
  };

  static void OnIsPublisherRegistered(
      CallbackHolder<IsPublisherRegisteredCallback>* holder,
      bool is_registered);

  static void OnPublisherInfo(CallbackHolder<GetPublisherInfoCallback>* holder,
                              const ledger::mojom::Result result,
                              ledger::mojom::PublisherInfoPtr info);

  static void OnPublisherPanelInfo(
      CallbackHolder<GetPublisherPanelInfoCallback>* holder,
      const ledger::mojom::Result result,
      ledger::mojom::PublisherInfoPtr info);

  static void OnGetBalanceReport(
      CallbackHolder<GetBalanceReportCallback>* holder,
      const ledger::mojom::Result result,
      ledger::mojom::BalanceReportInfoPtr report_info);

  static void OnInitialize(CallbackHolder<InitializeCallback>* holder,
                           ledger::mojom::Result result);

  static void OnRecoverWallet(CallbackHolder<RecoverWalletCallback>* holder,
                              ledger::mojom::Result result);

  static void OnGetPublisherBanner(
      CallbackHolder<GetPublisherBannerCallback>* holder,
      ledger::mojom::PublisherBannerPtr banner);

  static void OnRemoveRecurringTip(
      CallbackHolder<RemoveRecurringTipCallback>* holder,
      const ledger::mojom::Result result);

  static void OnOneTimeTip(CallbackHolder<OneTimeTipCallback>* holder,
                           const ledger::mojom::Result result);

  static void OnGetRewardsInternalsInfo(
      CallbackHolder<GetRewardsInternalsInfoCallback>* holder,
      ledger::mojom::RewardsInternalsInfoPtr info);

  static void OnSaveRecurringTip(
      CallbackHolder<SaveRecurringTipCallback>* holder,
      ledger::mojom::Result result);

  static void OnGetRecurringTips(
      CallbackHolder<GetRecurringTipsCallback>* holder,
      std::vector<ledger::mojom::PublisherInfoPtr> list);

  static void OnGetOneTimeTips(
      CallbackHolder<GetRecurringTipsCallback>* holder,
      std::vector<ledger::mojom::PublisherInfoPtr> list);
  static void OnRefreshPublisher(
      CallbackHolder<RefreshPublisherCallback>* holder,
      ledger::mojom::PublisherStatus status);

  static void OnGetActivityInfoList(
      CallbackHolder<GetActivityInfoListCallback>* holder,
      std::vector<ledger::mojom::PublisherInfoPtr> list);

  static void OnGetExcludedList(
      CallbackHolder<GetExcludedListCallback>* holder,
      std::vector<ledger::mojom::PublisherInfoPtr> list);

  static void OnGetPendingContributions(
      CallbackHolder<GetPendingContributionsCallback>* holder,
      std::vector<ledger::mojom::PendingContributionInfoPtr> list);

  static void OnRemovePendingContribution(
      CallbackHolder<RemovePendingContributionCallback>* holder,
      ledger::mojom::Result result);

  static void OnRemoveAllPendingContributions(
      CallbackHolder<RemovePendingContributionCallback>* holder,
      ledger::mojom::Result result);

  static void OnGetPendingContributionsTotal(
    CallbackHolder<GetPendingContributionsTotalCallback>* holder,
    double amount);

  static void OnHasSufficientBalanceToReconcile(
    CallbackHolder<HasSufficientBalanceToReconcileCallback>* holder,
    bool sufficient);

  static void OnExternalWalletAuthorization(
      CallbackHolder<ExternalWalletAuthorizationCallback>* holder,
      ledger::mojom::Result result,
      const base::flat_map<std::string, std::string>& args);

  static void OnDisconnectWallet(
      CallbackHolder<DisconnectWalletCallback>* holder,
      ledger::mojom::Result result);

  static void OnGetTransactionReport(
      CallbackHolder<GetTransactionReportCallback>* holder,
      std::vector<ledger::mojom::TransactionReportInfoPtr> list);

  static void OnGetContributionReport(
      CallbackHolder<GetContributionReportCallback>* holder,
      std::vector<ledger::mojom::ContributionReportInfoPtr> list);

  static void OnGetAllContributions(
      CallbackHolder<GetAllContributionsCallback>* holder,
      std::vector<ledger::mojom::ContributionInfoPtr> list);

  static void OnSavePublisherInfoForTip(
      CallbackHolder<SavePublisherInfoForTipCallback>* holder,
      const ledger::mojom::Result result);

  static void OnSavePublisherInfo(
      CallbackHolder<SavePublisherInfoCallback>* holder,
      const ledger::mojom::Result result);

  static void OnGetMonthlyReport(
      CallbackHolder<GetMonthlyReportCallback>* holder,
      const ledger::mojom::Result result,
      ledger::mojom::MonthlyReportInfoPtr info);

  static void OnGetAllMonthlyReportIds(
      CallbackHolder<GetAllMonthlyReportIdsCallback>* holder,
      const std::vector<std::string>& ids);

  static void OnGetAllPromotions(
      CallbackHolder<GetAllPromotionsCallback>* holder,
      base::flat_map<std::string, ledger::mojom::PromotionPtr> items);

  static void OnShutdown(CallbackHolder<ShutdownCallback>* holder,
                         const ledger::mojom::Result result);

  static void OnGetEventLogs(CallbackHolder<GetEventLogsCallback>* holder,
                             std::vector<ledger::mojom::EventLogPtr> logs);

  static void OnGetRewardsWallet(
      CallbackHolder<GetRewardsWalletCallback>* holder,
      ledger::mojom::RewardsWalletPtr wallet);

  std::unique_ptr<BatLedgerClientMojoBridge> bat_ledger_client_mojo_bridge_;
  std::unique_ptr<ledger::Ledger> ledger_;
};

}  // namespace bat_ledger

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_IMPL_H_
