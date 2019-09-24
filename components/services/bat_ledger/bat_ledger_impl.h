/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_IMPL_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "bat/ledger/ledger.h"
#include "brave/components/services/bat_ledger/public/interfaces/bat_ledger.mojom.h"

namespace bat_ledger {

class BatLedgerClientMojoProxy;

class BatLedgerImpl : public mojom::BatLedger,
    public base::SupportsWeakPtr<BatLedgerImpl> {
 public:
  explicit BatLedgerImpl(mojom::BatLedgerClientAssociatedPtrInfo client_info);
  ~BatLedgerImpl() override;

  // bat_ledger::mojom::BatLedger
  void Initialize(InitializeCallback callback) override;
  void CreateWallet(const std::string& safetynet_token,
      CreateWalletCallback callback) override;
  void FetchWalletProperties(FetchWalletPropertiesCallback callback) override;

  void GetAutoContributeProps(
      GetAutoContributePropsCallback callback) override;
  void GetPublisherMinVisitTime(
      GetPublisherMinVisitTimeCallback callback) override;
  void GetPublisherMinVisits(
      GetPublisherMinVisitsCallback callback) override;
  void GetPublisherAllowNonVerified(
      GetPublisherAllowNonVerifiedCallback callback) override;
  void GetPublisherAllowVideos(
      GetPublisherAllowVideosCallback callback) override;
  void GetAutoContribute(GetAutoContributeCallback callback) override;
  void GetReconcileStamp(GetReconcileStampCallback callback) override;

  void OnLoad(ledger::VisitDataPtr visit_data, uint64_t current_time) override;
  void OnUnload(uint32_t tab_id, uint64_t current_time) override;
  void OnShow(uint32_t tab_id, uint64_t current_time) override;
  void OnHide(uint32_t tab_id, uint64_t current_time) override;
  void OnForeground(uint32_t tab_id, uint64_t current_time) override;
  void OnBackground(uint32_t tab_id, uint64_t current_time) override;

  void OnPostData(const std::string& url,
      const std::string& first_party_url, const std::string& referrer,
      const std::string& post_data, ledger::VisitDataPtr visit_data) override;
  void OnXHRLoad(uint32_t tab_id, const std::string& url,
      const base::flat_map<std::string, std::string>& parts,
      const std::string& first_party_url, const std::string& referrer,
      ledger::VisitDataPtr visit_data) override;

  void SetPublisherExclude(
      const std::string& publisher_key,
      const int32_t exclude,
      SetPublisherExcludeCallback callback) override;
  void RestorePublishers(RestorePublishersCallback callback) override;

  void SetBalanceReportItem(
      int32_t month, int32_t year, int32_t type,
      const std::string& probi) override;
  void OnReconcileCompleteSuccess(
      const std::string& viewing_id,
      const ledger::RewardsCategory category,
      const std::string& probi,
      int32_t month,
      int32_t year,
      uint32_t data) override;

  void FetchGrants(
      const std::string& lang,
      const std::string& payment_id,
      const std::string& result_string,
      FetchGrantsCallback callback) override;
  void GetGrantCaptcha(const std::vector<std::string>& headers,
      GetGrantCaptchaCallback callback) override;
  void GetWalletPassphrase(GetWalletPassphraseCallback callback) override;
  void RecoverWallet(
      const std::string& pass_phrase,
      RecoverWalletCallback callback) override;
  void SolveGrantCaptcha(
      const std::string& solution,
      const std::string& promotionId) override;

  void SetRewardsMainEnabled(bool enabled) override;
  void SetPublisherMinVisitTime(uint64_t duration_in_seconds) override;
  void SetPublisherMinVisits(uint32_t visits) override;
  void SetPublisherAllowNonVerified(bool allow) override;
  void SetPublisherAllowVideos(bool allow) override;
  void SetUserChangedContribution() override;
  void SetContributionAmount(double amount) override;
  void SetAutoContribute(bool enabled) override;
  void UpdateAdsRewards() override;

  void OnTimer(uint32_t timer_id) override;

  void GetAllBalanceReports(GetAllBalanceReportsCallback callback) override;
  void GetBalanceReport(int32_t month, int32_t year,
      GetBalanceReportCallback callback) override;

  void IsWalletCreated(IsWalletCreatedCallback callback) override;

  void GetPublisherActivityFromUrl(
      uint64_t window_id,
      ledger::VisitDataPtr visit_data,
      const std::string& publisher_blob) override;

  void GetContributionAmount(
      GetContributionAmountCallback callback) override;
  void GetPublisherBanner(const std::string& publisher_id,
      GetPublisherBannerCallback callback) override;

  void DoDirectTip(const std::string& publisher_id,
                   int32_t amount,
                   const std::string& currency,
                   DoDirectTipCallback callback) override;

  void RemoveRecurringTip(
      const std::string& publisher_key,
      RemoveRecurringTipCallback callback) override;
  void GetBootStamp(GetBootStampCallback callback) override;
  void GetRewardsMainEnabled(
      GetRewardsMainEnabledCallback callback) override;
  void HasSufficientBalanceToReconcile(
      HasSufficientBalanceToReconcileCallback callback) override;

  void GetGrantViaSafetynetCheck(const std::string& promotion_id) override;
  void ApplySafetynetToken(
      const std::string& promotion_id,
      const std::string& result_string) override;

  void GetTransactionHistory(
      GetTransactionHistoryCallback callback) override;
  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;
  void RefreshPublisher(
      const std::string& publisher_key,
      RefreshPublisherCallback callback) override;
  void StartMonthlyContribution() override;

  void SaveRecurringTip(
      ledger::ContributionInfoPtr info,
      SaveRecurringTipCallback callback) override;
  void GetRecurringTips(GetRecurringTipsCallback callback) override;

  void GetOneTimeTips(GetOneTimeTipsCallback callback) override;

  void GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilterPtr filter,
    GetActivityInfoListCallback callback) override;

  void LoadPublisherInfo(
    const std::string& publisher_key,
    LoadPublisherInfoCallback callback) override;

  void SaveMediaInfo(
      const std::string& type,
      const base::flat_map<std::string, std::string>& args,
      SaveMediaInfoCallback callback) override;

  void SetInlineTipSetting(const std::string& key, bool enabled) override;

  void GetInlineTipSetting(
    const std::string& key,
    GetInlineTipSettingCallback callback) override;

  void GetShareURL(
    const std::string& type,
    const base::flat_map<std::string, std::string>& args,
    GetShareURLCallback callback) override;

  void GetPendingContributions(
    GetPendingContributionsCallback callback) override;

  void RemovePendingContribution(
    const std::string& publisher_key,
    const std::string& viewing_id,
    uint64_t added_date,
    RemovePendingContributionCallback callback) override;

  void RemoveAllPendingContributions(
    RemovePendingContributionCallback callback) override;

  void GetPendingContributionsTotal(
    GetPendingContributionsTotalCallback callback) override;

  void FetchBalance(FetchBalanceCallback callback) override;

  void GetExternalWallet(const std::string& wallet_type,
                         GetExternalWalletCallback callback) override;

  void ExternalWalletAuthorization(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ExternalWalletAuthorizationCallback callback) override;

  void DisconnectWallet(
    const std::string& wallet_type,
    DisconnectWalletCallback callback) override;

 private:
  void SetCatalogIssuers(const std::string& info) override;
  void ConfirmAd(const std::string& info) override;
  void ConfirmAction(const std::string& uuid,
                     const std::string& creative_set_id,
                     const std::string& type) override;

  // workaround to pass base::OnceCallback into std::bind
  template <typename Callback>
    class CallbackHolder {
     public:
      CallbackHolder(base::WeakPtr<BatLedgerImpl> client,
          Callback callback)
        : client_(client),
        callback_(std::move(callback)) {}
      ~CallbackHolder() = default;
      bool is_valid() { return !!client_.get(); }
      Callback& get() { return callback_; }

     private:
      base::WeakPtr<BatLedgerImpl> client_;
      Callback callback_;
    };

  static void OnGetBalanceReport(
      CallbackHolder<GetBalanceReportCallback>* holder,
      const bool result,
      ledger::BalanceReportInfoPtr report_info);

  static void OnGetGrantCaptcha(
      CallbackHolder<GetGrantCaptchaCallback>* holder,
      const std::string& image,
      const std::string& hint);

  static void OnCreateWallet(
      CallbackHolder<CreateWalletCallback>* holder,
      ledger::Result result);

  static void OnInitialize(
      CallbackHolder<InitializeCallback>* holder,
      ledger::Result result);

  static void OnRecoverWallet(
      CallbackHolder<RecoverWalletCallback>* holder,
      ledger::Result result,
      double balance,
      std::vector<ledger::GrantPtr> grants);

  static void OnFetchWalletProperties(
      CallbackHolder<FetchWalletPropertiesCallback>* holder,
      ledger::Result result,
      ledger::WalletPropertiesPtr properties);

  static void OnSetPublisherExclude(
      CallbackHolder<SetPublisherExcludeCallback>* holder,
      const ledger::Result result);

  static void OnRestorePublishers(
      CallbackHolder<SetPublisherExcludeCallback>* holder,
      const ledger::Result result);

  static void OnGetPublisherBanner(
      CallbackHolder<GetPublisherBannerCallback>* holder,
      ledger::PublisherBannerPtr banner);

  static void OnRemoveRecurringTip(
      CallbackHolder<RemoveRecurringTipCallback>* holder,
      const ledger::Result result);

  static void OnDoDirectTip(
      CallbackHolder<DoDirectTipCallback>* holder,
      const ledger::Result result);

  static void OnGetTransactionHistory(
      CallbackHolder<GetTransactionHistoryCallback>* holder,
      std::unique_ptr<ledger::TransactionsInfo> history);

  static void OnGetRewardsInternalsInfo(
      CallbackHolder<GetRewardsInternalsInfoCallback>* holder,
      ledger::RewardsInternalsInfoPtr info);

  static void OnSaveRecurringTip(
      CallbackHolder<SaveRecurringTipCallback>* holder,
      ledger::Result result);

  static void OnGetRecurringTips(
      CallbackHolder<GetRecurringTipsCallback>* holder,
      ledger::PublisherInfoList list,
      uint32_t num);

  static void OnGetOneTimeTips(
      CallbackHolder<GetRecurringTipsCallback>* holder,
      ledger::PublisherInfoList list,
      uint32_t num);
  static void OnRefreshPublisher(
      CallbackHolder<RefreshPublisherCallback>* holder,
      ledger::PublisherStatus status);

  static void OnGetActivityInfoList(
    CallbackHolder<GetActivityInfoListCallback>* holder,
    ledger::PublisherInfoList list,
    uint32_t num);

  static void OnLoadPublisherInfo(
    CallbackHolder<LoadPublisherInfoCallback>* holder,
    ledger::Result result,
    ledger::PublisherInfoPtr info);

  static void OnSaveMediaInfoCallback(
    CallbackHolder<SaveMediaInfoCallback>* holder,
    ledger::Result result,
    ledger::PublisherInfoPtr info);

  static void OnGetPendingContributions(
    CallbackHolder<GetPendingContributionsCallback>* holder,
    ledger::PendingContributionInfoList list);

  static void OnRemovePendingContribution(
    CallbackHolder<RemovePendingContributionCallback>* holder,
    ledger::Result result);

  static void OnRemoveAllPendingContributions(
    CallbackHolder<RemovePendingContributionCallback>* holder,
    ledger::Result result);

  static void OnGetPendingContributionsTotal(
    CallbackHolder<GetPendingContributionsTotalCallback>* holder,
    double amount);

  static void OnFetchGrants(
    CallbackHolder<FetchGrantsCallback>* holder,
    ledger::Result result,
    std::vector<ledger::GrantPtr> grants);

  static void OnHasSufficientBalanceToReconcile(
    CallbackHolder<HasSufficientBalanceToReconcileCallback>* holder,
    bool sufficient);

  static void OnFetchBalance(
      CallbackHolder<FetchBalanceCallback>* holder,
      ledger::Result result,
      ledger::BalancePtr balance);

  static void OnGetExternalWallet(
    CallbackHolder<GetExternalWalletCallback>* holder,
    ledger::Result result,
    ledger::ExternalWalletPtr wallet);

  static void OnExternalWalletAuthorization(
    CallbackHolder<ExternalWalletAuthorizationCallback>* holder,
    ledger::Result result,
    const std::map<std::string, std::string>& args);

  static void OnDisconnectWallet(
    CallbackHolder<DisconnectWalletCallback>* holder,
    ledger::Result result);

  std::unique_ptr<BatLedgerClientMojoProxy> bat_ledger_client_mojo_proxy_;
  std::unique_ptr<ledger::Ledger> ledger_;

  DISALLOW_COPY_AND_ASSIGN(BatLedgerImpl);
};

}  // namespace bat_ledger

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_IMPL_H_
