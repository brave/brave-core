/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_impl.h"

#include <map>
#include <string>
#include <utility>

#include "base/containers/flat_map.h"
#include "brave/components/services/bat_ledger/bat_ledger_client_mojo_proxy.h"
#include "mojo/public/cpp/bindings/map.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace bat_ledger {

namespace {  // TODO(Nejc Zdovc): Move into a util class

ledger::PUBLISHER_EXCLUDE ToLedgerPublisherExclude(int32_t exclude) {
  return (ledger::PUBLISHER_EXCLUDE)exclude;
}

ledger::ACTIVITY_MONTH ToLedgerPublisherMonth(int32_t month) {
  return (ledger::ACTIVITY_MONTH)month;
}

ledger::ReportType ToLedgerReportType(int32_t type) {
  return (ledger::ReportType)type;
}

ledger::REWARDS_CATEGORY ToLedgerPublisherCategory(int32_t category) {
  return (ledger::REWARDS_CATEGORY)category;
}

}  // namespace

BatLedgerImpl::BatLedgerImpl(
    mojom::BatLedgerClientAssociatedPtrInfo client_info)
  : bat_ledger_client_mojo_proxy_(
      new BatLedgerClientMojoProxy(std::move(client_info))),
    ledger_(
      ledger::Ledger::CreateInstance(bat_ledger_client_mojo_proxy_.get())) {
}

BatLedgerImpl::~BatLedgerImpl() {
}

void BatLedgerImpl::Initialize() {
  ledger_->Initialize();
}

void BatLedgerImpl::CreateWallet() {
  ledger_->CreateWallet();
}

// static
void BatLedgerImpl::OnFetchWalletProperties(
    CallbackHolder<FetchWalletPropertiesCallback>* holder,
    ledger::Result result,
    std::unique_ptr<ledger::WalletInfo> wallet_info) {
  std::string json_wallet = wallet_info.get() ? wallet_info->ToJson() : "";
  if (holder->is_valid())
    std::move(holder->get()).Run(result, json_wallet);
  delete holder;
}

void BatLedgerImpl::FetchWalletProperties(
    FetchWalletPropertiesCallback callback) {
  // delete in OnFetchWalletProperties
  auto* holder = new CallbackHolder<FetchWalletPropertiesCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->FetchWalletProperties(
      std::bind(BatLedgerImpl::OnFetchWalletProperties, holder, _1, _2));
}

void BatLedgerImpl::GetAutoContributeProps(
    GetAutoContributePropsCallback callback) {
  ledger::AutoContributeProps props;
  ledger_->GetAutoContributeProps(&props);
  std::move(callback).Run(props.ToJson());
}

void BatLedgerImpl::GetPublisherMinVisitTime(
    GetPublisherMinVisitTimeCallback callback) {
  std::move(callback).Run(ledger_->GetPublisherMinVisitTime());
}

void BatLedgerImpl::GetPublisherMinVisits(
    GetPublisherMinVisitsCallback callback) {
  std::move(callback).Run(ledger_->GetPublisherMinVisits());
}

void BatLedgerImpl::GetPublisherAllowNonVerified(
    GetPublisherAllowNonVerifiedCallback callback) {
  std::move(callback).Run(ledger_->GetPublisherAllowNonVerified());
}

void BatLedgerImpl::GetPublisherAllowVideos(
    GetPublisherAllowVideosCallback callback) {
  std::move(callback).Run(ledger_->GetPublisherAllowVideos());
}

void BatLedgerImpl::GetAutoContribute(
    GetAutoContributeCallback callback) {
  std::move(callback).Run(ledger_->GetAutoContribute());
}

void BatLedgerImpl::GetReconcileStamp(GetReconcileStampCallback callback) {
  std::move(callback).Run(ledger_->GetReconcileStamp());
}

void BatLedgerImpl::OnLoad(const std::string& visit_data,
    uint64_t current_time) {
  ledger::VisitData visitData;
  if (visitData.loadFromJson(visit_data))
    ledger_->OnLoad(visitData, current_time);
}

void BatLedgerImpl::OnUnload(uint32_t tab_id, uint64_t current_time) {
  ledger_->OnUnload(tab_id, current_time);
}

void BatLedgerImpl::OnShow(uint32_t tab_id, uint64_t current_time) {
  ledger_->OnShow(tab_id, current_time);
}

void BatLedgerImpl::OnHide(uint32_t tab_id, uint64_t current_time) {
  ledger_->OnHide(tab_id, current_time);
}

void BatLedgerImpl::OnForeground(uint32_t tab_id, uint64_t current_time) {
  ledger_->OnForeground(tab_id, current_time);
}

void BatLedgerImpl::OnBackground(uint32_t tab_id, uint64_t current_time) {
  ledger_->OnBackground(tab_id, current_time);
}

void BatLedgerImpl::OnMediaStart(uint32_t tab_id, uint64_t current_time) {
  ledger_->OnMediaStart(tab_id, current_time);
}

void BatLedgerImpl::OnMediaStop(uint32_t tab_id, uint64_t current_time) {
  ledger_->OnMediaStop(tab_id, current_time);
}

void BatLedgerImpl::OnPostData(const std::string& url,
    const std::string& first_party_url, const std::string& referrer,
    const std::string& post_data, const std::string& visit_data) {
  ledger::VisitData visitData;
  if (visitData.loadFromJson(visit_data))
    ledger_->OnPostData(url, first_party_url, referrer, post_data, visitData);
}

void BatLedgerImpl::OnXHRLoad(uint32_t tab_id, const std::string& url,
    const base::flat_map<std::string, std::string>& parts,
    const std::string& first_party_url, const std::string& referrer,
    const std::string& visit_data) {
  ledger::VisitData visitData;
  if (visitData.loadFromJson(visit_data))
    ledger_->OnXHRLoad(tab_id, url, mojo::FlatMapToMap(parts),
        first_party_url, referrer, visitData);
}

void BatLedgerImpl::SetPublisherExclude(const std::string& publisher_key,
    int32_t exclude) {
  ledger_->SetPublisherExclude(publisher_key,
      ToLedgerPublisherExclude(exclude));
}

void BatLedgerImpl::RestorePublishers() {
  ledger_->RestorePublishers();
}

void BatLedgerImpl::SetBalanceReportItem(int32_t month,
    int32_t year, int32_t type, const std::string& probi) {
  ledger_->SetBalanceReportItem(
      ToLedgerPublisherMonth(month), year, ToLedgerReportType(type), probi);
}

void BatLedgerImpl::OnReconcileCompleteSuccess(const std::string& viewing_id,
    int32_t category, const std::string& probi, int32_t month,
    int32_t year, uint32_t data) {
  ledger_->OnReconcileCompleteSuccess(viewing_id,
      ToLedgerPublisherCategory(category), probi,
      ToLedgerPublisherMonth(month), year, data);
}

void BatLedgerImpl::FetchGrants(const std::string& lang,
    const std::string& payment_id) {
  ledger_->FetchGrants(lang, payment_id);
}

void BatLedgerImpl::GetGrantCaptcha(
    const std::string& promotion_id,
    const std::string& promotion_type) {
  ledger_->GetGrantCaptcha(promotion_id, promotion_type);
}

void BatLedgerImpl::GetWalletPassphrase(GetWalletPassphraseCallback callback) {
  std::move(callback).Run(ledger_->GetWalletPassphrase());
}

// static
void BatLedgerImpl::OnGetExcludedPublishersNumber(
    CallbackHolder<GetExcludedPublishersNumberCallback>* holder,
    uint32_t number) {
  if (holder->is_valid())
    std::move(holder->get()).Run(number);
  delete holder;
}

void BatLedgerImpl::GetExcludedPublishersNumber(
    GetExcludedPublishersNumberCallback callback) {
  // delete in OnGetExcludedPublishersNumber
  auto* holder = new CallbackHolder<GetExcludedPublishersNumberCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->GetExcludedPublishersNumber(
      std::bind(BatLedgerImpl::OnGetExcludedPublishersNumber, holder, _1));
}

void BatLedgerImpl::RecoverWallet(const std::string& passPhrase) {
  ledger_->RecoverWallet(passPhrase);
}

void BatLedgerImpl::SolveGrantCaptcha(const std::string& solution,
                                      const std::string& promotion_id) {
  ledger_->SolveGrantCaptcha(solution, promotion_id);
}

void BatLedgerImpl::GetAddresses(GetAddressesCallback callback) {
  std::move(callback).Run(mojo::MapToFlatMap(ledger_->GetAddresses()));
}

void BatLedgerImpl::GetBATAddress(GetBATAddressCallback callback) {
  std::move(callback).Run(ledger_->GetBATAddress());
}

void BatLedgerImpl::GetBTCAddress(GetBTCAddressCallback callback) {
  std::move(callback).Run(ledger_->GetBTCAddress());
}

void BatLedgerImpl::GetETHAddress(GetETHAddressCallback callback) {
  std::move(callback).Run(ledger_->GetETHAddress());
}

void BatLedgerImpl::GetLTCAddress(GetLTCAddressCallback callback) {
  std::move(callback).Run(ledger_->GetLTCAddress());
}

void BatLedgerImpl::SetRewardsMainEnabled(bool enabled) {
  ledger_->SetRewardsMainEnabled(enabled);
}

void BatLedgerImpl::SetPublisherMinVisitTime(uint64_t duration_in_seconds) {
  ledger_->SetPublisherMinVisitTime(duration_in_seconds);
}

void BatLedgerImpl::SetPublisherMinVisits(uint32_t visits) {
  ledger_->SetPublisherMinVisits(visits);
}

void BatLedgerImpl::SetPublisherAllowNonVerified(bool allow) {
  ledger_->SetPublisherAllowNonVerified(allow);
}

void BatLedgerImpl::SetPublisherAllowVideos(bool allow) {
  ledger_->SetPublisherAllowVideos(allow);
}

void BatLedgerImpl::SetUserChangedContribution() {
  ledger_->SetUserChangedContribution();
}

void BatLedgerImpl::SetContributionAmount(double amount) {
  ledger_->SetContributionAmount(amount);
}

void BatLedgerImpl::SetAutoContribute(bool enabled) {
  ledger_->SetAutoContribute(enabled);
}

void BatLedgerImpl::OnTimer(uint32_t timer_id) {
  ledger_->OnTimer(timer_id);
}

void BatLedgerImpl::GetAllBalanceReports(
    GetAllBalanceReportsCallback callback) {
  auto reports = ledger_->GetAllBalanceReports();
  base::flat_map<std::string, std::string> out_reports;
  for (auto const& report : reports) {
    out_reports[report.first] = report.second.ToJson();
  }
  std::move(callback).Run(out_reports);
}

void BatLedgerImpl::GetBalanceReport(int32_t month, int32_t year,
    GetBalanceReportCallback callback) {
  ledger::BalanceReportInfo info;
  bool result =
    ledger_->GetBalanceReport(ToLedgerPublisherMonth(month), year, &info);
  std::move(callback).Run(result, info.ToJson());
}

void BatLedgerImpl::IsWalletCreated(IsWalletCreatedCallback callback) {
  std::move(callback).Run(ledger_->IsWalletCreated());
}

void BatLedgerImpl::GetPublisherActivityFromUrl(
    uint64_t window_id,
    const std::string& visit_data,
    const std::string& publisher_blob) {
  ledger::VisitData visitData;
  if (visitData.loadFromJson(visit_data))
    ledger_->GetPublisherActivityFromUrl(window_id, visitData, publisher_blob);
}

// static
void BatLedgerImpl::OnGetPublisherBanner(
    CallbackHolder<GetPublisherBannerCallback>* holder,
    std::unique_ptr<ledger::PublisherBanner> banner) {
  std::string json_banner = banner.get() ? banner->ToJson() : "";
  if (holder->is_valid())
    std::move(holder->get()).Run(json_banner);
  delete holder;
}

void BatLedgerImpl::GetPublisherBanner(const std::string& publisher_id,
    GetPublisherBannerCallback callback) {
  // delete in OnGetPublisherBanner
  auto* holder = new CallbackHolder<GetPublisherBannerCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->GetPublisherBanner(publisher_id,
      std::bind(BatLedgerImpl::OnGetPublisherBanner, holder, _1));
}

void BatLedgerImpl::GetContributionAmount(
    GetContributionAmountCallback callback) {
  std::move(callback).Run(ledger_->GetContributionAmount());
}

void BatLedgerImpl::DoDirectDonation(const std::string& publisher_info,
                                     int32_t amount,
                                     const std::string& currency) {
  ledger::PublisherInfo info;
  if (info.loadFromJson(publisher_info))
    ledger_->DoDirectDonation(info, amount, currency);
}

void BatLedgerImpl::RemoveRecurringTip(const std::string& publisher_key) {
  ledger_->RemoveRecurringTip(publisher_key);
}


void BatLedgerImpl::GetBootStamp(GetBootStampCallback callback) {
  std::move(callback).Run(ledger_->GetBootStamp());
}

void BatLedgerImpl::GetRewardsMainEnabled(
    GetRewardsMainEnabledCallback callback) {
  std::move(callback).Run(ledger_->GetRewardsMainEnabled());
}

void BatLedgerImpl::HasSufficientBalanceToReconcile(
    HasSufficientBalanceToReconcileCallback callback) {
  std::move(callback).Run(ledger_->HasSufficientBalanceToReconcile());
}

// static
void BatLedgerImpl::OnAddressesForPaymentId(
    CallbackHolder<GetAddressesForPaymentIdCallback>* holder,
    std::map<std::string, std::string> addresses) {
  if (holder->is_valid())
    std::move(holder->get()).Run(mojo::MapToFlatMap(addresses));
  delete holder;
}

void BatLedgerImpl::GetAddressesForPaymentId(
    GetAddressesForPaymentIdCallback callback) {
  // delete in OnAddressesForPaymentId
  auto* holder = new CallbackHolder<GetAddressesForPaymentIdCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->GetAddressesForPaymentId(
      std::bind(BatLedgerImpl::OnAddressesForPaymentId, holder, _1));
}

void BatLedgerImpl::SetCatalogIssuers(const std::string& info) {
  ledger_->SetCatalogIssuers(info);
}

void BatLedgerImpl::ConfirmAd(const std::string& info) {
  ledger_->ConfirmAd(info);
}

// static
void BatLedgerImpl::OnGetConfirmationsHistory(
    CallbackHolder<GetConfirmationsHistoryCallback>* holder,
    std::unique_ptr<ledger::TransactionsInfo> history) {
  std::string json_transactions = history.get() ? history->ToJson() : "";
  if (holder->is_valid())
    std::move(holder->get()).Run(json_transactions);
  delete holder;
}

void BatLedgerImpl::GetConfirmationsHistory(
    const uint64_t from_timestamp_seconds,
    const uint64_t to_timestamp_seconds,
    GetConfirmationsHistoryCallback callback) {
  auto* holder = new CallbackHolder<GetConfirmationsHistoryCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetConfirmationsHistory(from_timestamp_seconds,
      to_timestamp_seconds, std::bind(
      BatLedgerImpl::OnGetConfirmationsHistory, holder, _1));
}

void BatLedgerImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  ledger::RewardsInternalsInfo info;
  ledger_->GetRewardsInternalsInfo(&info);
  std::move(callback).Run(info.ToJson());
}

// static
void BatLedgerImpl::OnGetRecurringTips(
    CallbackHolder<GetRecurringTipsCallback>* holder,
    const ledger::PublisherInfoList& list,
    uint32_t num) {

  std::vector<std::string> json_list;
  for (auto const& item : list) {
    json_list.push_back(item.ToJson());
  }

  if (holder->is_valid()) {
    std::move(holder->get()).Run(json_list);
  }
  delete holder;
}

void BatLedgerImpl::GetRecurringTips(GetRecurringTipsCallback callback) {
  auto* holder = new CallbackHolder<GetRecurringTipsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetRecurringTips(std::bind(
      BatLedgerImpl::OnGetRecurringTips, holder, _1, _2));
}

}  // namespace bat_ledger
