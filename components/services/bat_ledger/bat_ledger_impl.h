/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_IMPL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <utility>

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
  void Initialize() override;
  void CreateWallet() override;
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

  void OnLoad(const std::string& visit_data, uint64_t current_time) override;
  void OnUnload(uint32_t tab_id, uint64_t current_time) override;
  void OnShow(uint32_t tab_id, uint64_t current_time) override;
  void OnHide(uint32_t tab_id, uint64_t current_time) override;
  void OnForeground(uint32_t tab_id, uint64_t current_time) override;
  void OnBackground(uint32_t tab_id, uint64_t current_time) override;
  void OnMediaStart(uint32_t tab_id, uint64_t current_time) override;
  void OnMediaStop(uint32_t tab_id, uint64_t current_time) override;

  void OnPostData(const std::string& url,
      const std::string& first_party_url, const std::string& referrer,
      const std::string& post_data, const std::string& visit_data) override;
  void OnXHRLoad(uint32_t tab_id, const std::string& url,
      const base::flat_map<std::string, std::string>& parts,
      const std::string& first_party_url, const std::string& referrer,
      const std::string& visit_data) override;

  void SetPublisherExclude(const std::string& publisher_key,
      int32_t exclude) override;
  void RestorePublishers() override;

  void SetBalanceReportItem(
      int32_t month, int32_t year, int32_t type,
      const std::string& probi) override;
  void OnReconcileCompleteSuccess(const std::string& viewing_id,
      int32_t category, const std::string& probi, int32_t month,
      int32_t year, uint32_t data) override;

  void FetchGrants(
      const std::string& lang, const std::string& payment_id) override;
  void GetGrantCaptcha(
    const std::string& promotion_id,
    const std::string& promotion_type) override;
  void GetWalletPassphrase(GetWalletPassphraseCallback callback) override;
  void GetExcludedPublishersNumber(GetExcludedPublishersNumberCallback callback) override;
  void RecoverWallet(const std::string& passPhrase) override;
  void SolveGrantCaptcha(
      const std::string& solution,
      const std::string& promotionId) override;

  void GetAddresses(GetAddressesCallback callback) override;
  void GetBATAddress(GetBATAddressCallback callback) override;
  void GetBTCAddress(GetBTCAddressCallback callback) override;
  void GetETHAddress(GetETHAddressCallback callback) override;
  void GetLTCAddress(GetLTCAddressCallback callback) override;

  void SetRewardsMainEnabled(bool enabled) override;
  void SetPublisherMinVisitTime(uint64_t duration_in_seconds) override;
  void SetPublisherMinVisits(uint32_t visits) override;
  void SetPublisherAllowNonVerified(bool allow) override;
  void SetPublisherAllowVideos(bool allow) override;
  void SetUserChangedContribution() override;
  void SetContributionAmount(double amount) override;
  void SetAutoContribute(bool enabled) override;

  void OnTimer(uint32_t timer_id) override;

  void GetAllBalanceReports(GetAllBalanceReportsCallback callback) override;
  void GetBalanceReport(int32_t month, int32_t year,
      GetBalanceReportCallback callback) override;

  void IsWalletCreated(IsWalletCreatedCallback callback) override;

  void GetPublisherActivityFromUrl(
      uint64_t window_id,
      const std::string& visit_data,
      const std::string& publisher_blob) override;

  void GetContributionAmount(
      GetContributionAmountCallback callback) override;
  void GetPublisherBanner(const std::string& publisher_id,
      GetPublisherBannerCallback callback) override;

  void DoDirectDonation(const std::string& publisher_info, int32_t amount,
      const std::string& currency) override;

  void RemoveRecurringTip(const std::string& publisher_key) override;
  void GetBootStamp(GetBootStampCallback callback) override;
  void GetRewardsMainEnabled(
      GetRewardsMainEnabledCallback callback) override;
  void HasSufficientBalanceToReconcile(
      HasSufficientBalanceToReconcileCallback callback) override;

  void GetAddressesForPaymentId(
      GetAddressesForPaymentIdCallback callback) override;
  void GetConfirmationsHistory(
      const uint64_t from_timestamp_seconds,
      const uint64_t to_timestamp_seconds,
      GetConfirmationsHistoryCallback callback) override;
  void GetRewardsInternalsInfo(
      GetRewardsInternalsInfoCallback callback) override;

  void GetRecurringTips(GetRecurringTipsCallback callback) override;

 private:
  void SetCatalogIssuers(const std::string& info) override;
  void ConfirmAd(const std::string& info) override;

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

  static void OnFetchWalletProperties(
      CallbackHolder<FetchWalletPropertiesCallback>* holder,
      ledger::Result result,
      std::unique_ptr<ledger::WalletInfo> wallet_info);

  static void OnGetPublisherBanner(
      CallbackHolder<GetPublisherBannerCallback>* holder,
      std::unique_ptr<ledger::PublisherBanner> banner);

  static void OnAddressesForPaymentId(
      CallbackHolder<GetAddressesForPaymentIdCallback>* holder,
      std::map<std::string, std::string> addresses);

  static void OnGetConfirmationsHistory(
      CallbackHolder<GetConfirmationsHistoryCallback>* holder,
      std::unique_ptr<ledger::TransactionsInfo> history);

  static void OnGetExcludedPublishersNumber(
      CallbackHolder<GetExcludedPublishersNumberCallback>* holder,
      uint32_t number);

  static void OnGetRecurringTips(
      CallbackHolder<GetRecurringTipsCallback>* holder,
      const ledger::PublisherInfoList& list,
      uint32_t num);

  std::unique_ptr<BatLedgerClientMojoProxy> bat_ledger_client_mojo_proxy_;
  std::unique_ptr<ledger::Ledger> ledger_;

  DISALLOW_COPY_AND_ASSIGN(BatLedgerImpl);
};

}  // namespace bat_ledger

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_LEDGER_BAT_LEDGER_IMPL_H_
