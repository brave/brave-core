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
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"

namespace brave_rewards {

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

  // mojom::BatLedger
  void Initialize(
    const bool execute_create_script,
    InitializeCallback callback) override;
  void CreateRewardsWallet(const std::string& country,
                           CreateRewardsWalletCallback callback) override;
  void GetRewardsParameters(GetRewardsParametersCallback callback) override;

  void GetAutoContributeProperties(
      GetAutoContributePropertiesCallback callback) override;
  void GetPublisherMinVisitTime(
      GetPublisherMinVisitTimeCallback callback) override;
  void GetPublisherMinVisits(
      GetPublisherMinVisitsCallback callback) override;
  void GetPublisherAllowNonVerified(
      GetPublisherAllowNonVerifiedCallback callback) override;
  void GetAutoContributeEnabled(
      GetAutoContributeEnabledCallback callback) override;
  void GetReconcileStamp(GetReconcileStampCallback callback) override;

  void OnLoad(mojom::VisitDataPtr visit_data, uint64_t current_time) override;
  void OnUnload(uint32_t tab_id, uint64_t current_time) override;
  void OnShow(uint32_t tab_id, uint64_t current_time) override;
  void OnHide(uint32_t tab_id, uint64_t current_time) override;
  void OnForeground(uint32_t tab_id, uint64_t current_time) override;
  void OnBackground(uint32_t tab_id, uint64_t current_time) override;
  void OnXHRLoad(uint32_t tab_id,
                 const std::string& url,
                 const base::flat_map<std::string, std::string>& parts,
                 const std::string& first_party_url,
                 const std::string& referrer,
                 mojom::VisitDataPtr visit_data) override;

  void SetPublisherExclude(const std::string& publisher_key,
                           mojom::PublisherExclude exclude,
                           SetPublisherExcludeCallback callback) override;
  void RestorePublishers(RestorePublishersCallback callback) override;

  void FetchPromotions(FetchPromotionsCallback callback) override;
  void ClaimPromotion(
      const std::string& promotion_id,
      const std::string& payload,
      ClaimPromotionCallback callback) override;
  void AttestPromotion(const std::string& promotion_id,
                       const std::string& solution,
                       AttestPromotionCallback callback) override;

  void SetPublisherMinVisitTime(int duration_in_seconds) override;
  void SetPublisherMinVisits(int visits) override;
  void SetPublisherAllowNonVerified(bool allow) override;
  void SetAutoContributionAmount(double amount) override;
  void SetAutoContributeEnabled(bool enabled) override;

  void GetBalanceReport(mojom::ActivityMonth month,
                        int32_t year,
                        GetBalanceReportCallback callback) override;

  void GetPublisherActivityFromUrl(uint64_t window_id,
                                   mojom::VisitDataPtr visit_data,
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

  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;
  void RefreshPublisher(
      const std::string& publisher_key,
      RefreshPublisherCallback callback) override;

  void StartContributionsForTesting() override;

  void SendContribution(const std::string& publisher_id,
                        double amount,
                        bool set_monthly,
                        SendContributionCallback callback) override;

  void SaveRecurringTip(mojom::RecurringTipPtr info,
                        SaveRecurringTipCallback callback) override;

  void GetRecurringTips(GetRecurringTipsCallback callback) override;

  void GetOneTimeTips(GetOneTimeTipsCallback callback) override;

  void GetActivityInfoList(uint32_t start,
                           uint32_t limit,
                           mojom::ActivityInfoFilterPtr filter,
                           GetActivityInfoListCallback callback) override;

  void GetPublishersVisitedCount(
      GetPublishersVisitedCountCallback callback) override;

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
                         mojom::PublisherInfoPtr publisher_info,
                         SavePublisherInfoCallback callback) override;

  void SetInlineTippingPlatformEnabled(
      const mojom::InlineTipsPlatforms platform,
      bool enabled) override;

  void GetInlineTippingPlatformEnabled(
      const mojom::InlineTipsPlatforms platform,
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

  void ConnectExternalWallet(
      const std::string& wallet_type,
      const base::flat_map<std::string, std::string>& args,
      ConnectExternalWalletCallback) override;

  void GetTransactionReport(const mojom::ActivityMonth month,
                            const int year,
                            GetTransactionReportCallback callback) override;

  void GetContributionReport(const mojom::ActivityMonth month,
                             const int year,
                             GetContributionReportCallback callback) override;

  void GetAllContributions(GetAllContributionsCallback callback) override;

  void SavePublisherInfoForTip(
      mojom::PublisherInfoPtr info,
      SavePublisherInfoForTipCallback callback) override;

  void GetMonthlyReport(const mojom::ActivityMonth month,
                        const int year,
                        GetMonthlyReportCallback callback) override;

  void GetAllMonthlyReportIds(
      GetAllMonthlyReportIdsCallback callback) override;

  void GetAllPromotions(GetAllPromotionsCallback callback) override;

  void Shutdown(ShutdownCallback callback) override;

  void GetEventLogs(GetEventLogsCallback callback) override;

  void GetRewardsWallet(GetRewardsWalletCallback callback) override;

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
                              const mojom::Result result,
                              mojom::PublisherInfoPtr info);

  static void OnPublisherPanelInfo(
      CallbackHolder<GetPublisherPanelInfoCallback>* holder,
      const mojom::Result result,
      mojom::PublisherInfoPtr info);

  static void OnGetBalanceReport(
      CallbackHolder<GetBalanceReportCallback>* holder,
      const mojom::Result result,
      mojom::BalanceReportInfoPtr report_info);

  static void OnInitialize(CallbackHolder<InitializeCallback>* holder,
                           mojom::Result result);

  static void OnGetPublisherBanner(
      CallbackHolder<GetPublisherBannerCallback>* holder,
      mojom::PublisherBannerPtr banner);

  static void OnRemoveRecurringTip(
      CallbackHolder<RemoveRecurringTipCallback>* holder,
      const mojom::Result result);

  static void OnOneTimeTip(CallbackHolder<OneTimeTipCallback>* holder,
                           const mojom::Result result);

  static void OnGetRewardsInternalsInfo(
      CallbackHolder<GetRewardsInternalsInfoCallback>* holder,
      mojom::RewardsInternalsInfoPtr info);

  static void OnSaveRecurringTip(
      CallbackHolder<SaveRecurringTipCallback>* holder,
      mojom::Result result);

  static void OnGetRecurringTips(
      CallbackHolder<GetRecurringTipsCallback>* holder,
      std::vector<mojom::PublisherInfoPtr> list);

  static void OnGetOneTimeTips(CallbackHolder<GetRecurringTipsCallback>* holder,
                               std::vector<mojom::PublisherInfoPtr> list);
  static void OnRefreshPublisher(
      CallbackHolder<RefreshPublisherCallback>* holder,
      mojom::PublisherStatus status);

  static void OnGetActivityInfoList(
      CallbackHolder<GetActivityInfoListCallback>* holder,
      std::vector<mojom::PublisherInfoPtr> list);

  static void OnGetExcludedList(CallbackHolder<GetExcludedListCallback>* holder,
                                std::vector<mojom::PublisherInfoPtr> list);

  static void OnGetPendingContributions(
      CallbackHolder<GetPendingContributionsCallback>* holder,
      std::vector<mojom::PendingContributionInfoPtr> list);

  static void OnRemovePendingContribution(
      CallbackHolder<RemovePendingContributionCallback>* holder,
      mojom::Result result);

  static void OnRemoveAllPendingContributions(
      CallbackHolder<RemovePendingContributionCallback>* holder,
      mojom::Result result);

  static void OnGetPendingContributionsTotal(
    CallbackHolder<GetPendingContributionsTotalCallback>* holder,
    double amount);

  static void OnGetTransactionReport(
      CallbackHolder<GetTransactionReportCallback>* holder,
      std::vector<mojom::TransactionReportInfoPtr> list);

  static void OnGetContributionReport(
      CallbackHolder<GetContributionReportCallback>* holder,
      std::vector<mojom::ContributionReportInfoPtr> list);

  static void OnGetAllContributions(
      CallbackHolder<GetAllContributionsCallback>* holder,
      std::vector<mojom::ContributionInfoPtr> list);

  static void OnSavePublisherInfoForTip(
      CallbackHolder<SavePublisherInfoForTipCallback>* holder,
      const mojom::Result result);

  static void OnSavePublisherInfo(
      CallbackHolder<SavePublisherInfoCallback>* holder,
      const mojom::Result result);

  static void OnGetMonthlyReport(
      CallbackHolder<GetMonthlyReportCallback>* holder,
      const mojom::Result result,
      mojom::MonthlyReportInfoPtr info);

  static void OnGetAllMonthlyReportIds(
      CallbackHolder<GetAllMonthlyReportIdsCallback>* holder,
      const std::vector<std::string>& ids);

  static void OnGetAllPromotions(
      CallbackHolder<GetAllPromotionsCallback>* holder,
      base::flat_map<std::string, mojom::PromotionPtr> items);

  static void OnShutdown(CallbackHolder<ShutdownCallback>* holder,
                         const mojom::Result result);

  static void OnGetEventLogs(CallbackHolder<GetEventLogsCallback>* holder,
                             std::vector<mojom::EventLogPtr> logs);

  static void OnGetRewardsWallet(
      CallbackHolder<GetRewardsWalletCallback>* holder,
      mojom::RewardsWalletPtr wallet);

  std::unique_ptr<BatLedgerClientMojoBridge> bat_ledger_client_mojo_bridge_;
  std::unique_ptr<core::Ledger> ledger_;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_IMPL_H_
