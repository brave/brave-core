/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_impl.h"

#include <stdint.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/base/containers/utils.h"
#include "brave/components/services/bat_ledger/bat_ledger_client_mojo_proxy.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace bat_ledger {

BatLedgerImpl::BatLedgerImpl(
    mojom::BatLedgerClientAssociatedPtrInfo client_info)
  : bat_ledger_client_mojo_proxy_(
      new BatLedgerClientMojoProxy(std::move(client_info))),
    ledger_(
      ledger::Ledger::CreateInstance(bat_ledger_client_mojo_proxy_.get())) {
}

BatLedgerImpl::~BatLedgerImpl() {
}


void BatLedgerImpl::OnInitialize(
    CallbackHolder<InitializeCallback>* holder,
    ledger::Result result) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}
void BatLedgerImpl::Initialize(InitializeCallback callback) {
  auto* holder = new CallbackHolder<InitializeCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->Initialize(
      std::bind(BatLedgerImpl::OnInitialize, holder, _1));
}

// static
void BatLedgerImpl::OnCreateWallet(
    CallbackHolder<CreateWalletCallback>* holder,
    ledger::Result result) {
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void BatLedgerImpl::CreateWallet(const std::string& safetynet_token,
    CreateWalletCallback callback) {
  // deleted in OnCreateWallet
  auto* holder = new CallbackHolder<CreateWalletCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->CreateWallet(safetynet_token,
      std::bind(BatLedgerImpl::OnCreateWallet, holder, _1));
}

// static
void BatLedgerImpl::OnFetchWalletProperties(
    CallbackHolder<FetchWalletPropertiesCallback>* holder,
    ledger::Result result,
    ledger::WalletPropertiesPtr properties) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(properties));
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
  ledger::AutoContributePropsPtr props = ledger_->GetAutoContributeProps();
  std::move(callback).Run(std::move(props));
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

void BatLedgerImpl::OnLoad(ledger::VisitDataPtr visit_data,
    uint64_t current_time) {
  ledger_->OnLoad(std::move(visit_data), current_time);
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

void BatLedgerImpl::OnPostData(const std::string& url,
    const std::string& first_party_url, const std::string& referrer,
    const std::string& post_data, ledger::VisitDataPtr visit_data) {
  ledger_->OnPostData(
      url, first_party_url, referrer, post_data, std::move(visit_data));
}

void BatLedgerImpl::OnXHRLoad(uint32_t tab_id, const std::string& url,
    const base::flat_map<std::string, std::string>& parts,
    const std::string& first_party_url, const std::string& referrer,
    ledger::VisitDataPtr visit_data) {
    ledger_->OnXHRLoad(tab_id, url, base::FlatMapToMap(parts),
        first_party_url, referrer, std::move(visit_data));
}

// static
void BatLedgerImpl::OnSetPublisherExclude(
    CallbackHolder<SetPublisherExcludeCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void BatLedgerImpl::SetPublisherExclude(
    const std::string& publisher_key,
    const ledger::PublisherExclude exclude,
    SetPublisherExcludeCallback callback) {
  // delete in OnSetPublisherExclude
  auto* holder = new CallbackHolder<SetPublisherExcludeCallback>(
    AsWeakPtr(), std::move(callback));

  ledger_->SetPublisherExclude(
    publisher_key,
    exclude,
    std::bind(BatLedgerImpl::OnSetPublisherExclude, holder, _1));
}

// static
void BatLedgerImpl::OnRestorePublishers(
  CallbackHolder<SetPublisherExcludeCallback>* holder,
  const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void BatLedgerImpl::RestorePublishers(RestorePublishersCallback callback) {
  // delete in OnRestorePublishers
  auto* holder = new CallbackHolder<RestorePublishersCallback>(
    AsWeakPtr(), std::move(callback));
  ledger_->RestorePublishers(
    std::bind(BatLedgerImpl::OnRestorePublishers, holder, _1));
}

void BatLedgerImpl::SetBalanceReportItem(
    ledger::ActivityMonth month,
    int32_t year,
    ledger::ReportType type,
    const std::string& probi) {
  ledger_->SetBalanceReportItem(month, year, type, probi);
}

void BatLedgerImpl::OnFetchPromotions(
    CallbackHolder<FetchPromotionsCallback>* holder,
    const ledger::Result result,
    ledger::PromotionList promotions) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result, std::move(promotions));
  }
  delete holder;
}

void BatLedgerImpl::FetchPromotions(
    FetchPromotionsCallback callback) {
  // deleted in OnFetchPromotions
  auto* holder = new CallbackHolder<FetchPromotionsCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->FetchPromotions(
      std::bind(BatLedgerImpl::OnFetchPromotions, holder, _1, _2));
}

// static
void BatLedgerImpl::OnClaimPromotion(
    CallbackHolder<ClaimPromotionCallback>* holder,
    const ledger::Result result,
    const std::string& response) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, response);
  delete holder;
}
void BatLedgerImpl::ClaimPromotion(
    const std::string& payload,
    ClaimPromotionCallback callback) {
  // deleted in OnClaimPromotion
  auto* holder = new CallbackHolder<ClaimPromotionCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->ClaimPromotion(
      payload,
      std::bind(BatLedgerImpl::OnClaimPromotion, holder, _1, _2));
}

// static
void BatLedgerImpl::OnAttestPromotion(
    CallbackHolder<AttestPromotionCallback>* holder,
    const ledger::Result result,
    ledger::PromotionPtr promotion) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(promotion));
  delete holder;
}
void BatLedgerImpl::AttestPromotion(
    const std::string& promotion_id,
    const std::string& solution,
    AttestPromotionCallback callback) {
  // deleted in OnAttestPromotion
  auto* holder = new CallbackHolder<AttestPromotionCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->AttestPromotion(
      promotion_id,
      solution,
      std::bind(BatLedgerImpl::OnAttestPromotion, holder, _1, _2));
}

void BatLedgerImpl::GetWalletPassphrase(GetWalletPassphraseCallback callback) {
  std::move(callback).Run(ledger_->GetWalletPassphrase());
}

// static
void BatLedgerImpl::OnRecoverWallet(
    CallbackHolder<RecoverWalletCallback>* holder,
    ledger::Result result,
    double balance) {
  if (holder->is_valid())
    std::move(holder->get()).Run(result, balance);
  delete holder;
}

void BatLedgerImpl::RecoverWallet(
    const std::string& pass_phrase,
    RecoverWalletCallback callback) {
  // deleted in OnRecoverWallet
  auto* holder = new CallbackHolder<RecoverWalletCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->RecoverWallet(pass_phrase, std::bind(
      BatLedgerImpl::OnRecoverWallet,
      holder,
      _1,
      _2));
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

void BatLedgerImpl::UpdateAdsRewards() {
  ledger_->UpdateAdsRewards();
}

void BatLedgerImpl::OnTimer(uint32_t timer_id) {
  ledger_->OnTimer(timer_id);
}

void BatLedgerImpl::GetAllBalanceReports(
    GetAllBalanceReportsCallback callback) {
  std::map<std::string, ledger::BalanceReportInfoPtr> reports =
    ledger_->GetAllBalanceReports();
  auto out_reports = base::MapToFlatMap(std::move(reports));
  std::move(callback).Run(std::move(out_reports));
}

// static
void BatLedgerImpl::OnGetBalanceReport(
    CallbackHolder<GetBalanceReportCallback>* holder,
    const bool result,
    ledger::BalanceReportInfoPtr report_info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(report_info));
  delete holder;
}
void BatLedgerImpl::GetBalanceReport(ledger::ActivityMonth month, int32_t year,
    GetBalanceReportCallback callback) {
  auto* holder = new CallbackHolder<GetBalanceReportCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->GetBalanceReport(
      month,
      year,
      std::bind(BatLedgerImpl::OnGetBalanceReport, holder, _1, _2));
}

void BatLedgerImpl::IsWalletCreated(IsWalletCreatedCallback callback) {
  std::move(callback).Run(ledger_->IsWalletCreated());
}

void BatLedgerImpl::GetPublisherActivityFromUrl(
    uint64_t window_id,
    ledger::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  ledger_->GetPublisherActivityFromUrl(
      window_id, std::move(visit_data), publisher_blob);
}

// static
void BatLedgerImpl::OnGetPublisherBanner(
    CallbackHolder<GetPublisherBannerCallback>* holder,
    ledger::PublisherBannerPtr banner) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(banner));
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

void BatLedgerImpl::OnDoDirectTip(
    CallbackHolder<DoDirectTipCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void BatLedgerImpl::DoDirectTip(const std::string& publisher_id,
                                double amount,
                                const std::string& currency,
                                DoDirectTipCallback callback) {
  // deleted in OnDoDirectTip
  auto* holder = new CallbackHolder<DoDirectTipCallback>(
    AsWeakPtr(), std::move(callback));
  ledger_->DoDirectTip(publisher_id, amount, currency,
    std::bind(BatLedgerImpl::OnDoDirectTip, holder, _1));
}

// static
void BatLedgerImpl::OnRemoveRecurringTip(
    CallbackHolder<RemoveRecurringTipCallback>* holder,
    const ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void BatLedgerImpl::RemoveRecurringTip(
  const std::string& publisher_key,
  RemoveRecurringTipCallback callback) {
  auto* holder = new CallbackHolder<RemoveRecurringTipCallback>(
            AsWeakPtr(), std::move(callback));
  ledger_->RemoveRecurringTip(
    publisher_key,
    std::bind(BatLedgerImpl::OnRemoveRecurringTip, holder, _1));
}

void BatLedgerImpl::GetBootStamp(GetBootStampCallback callback) {
  std::move(callback).Run(ledger_->GetBootStamp());
}

void BatLedgerImpl::GetRewardsMainEnabled(
    GetRewardsMainEnabledCallback callback) {
  std::move(callback).Run(ledger_->GetRewardsMainEnabled());
}

void BatLedgerImpl::OnHasSufficientBalanceToReconcile(
    CallbackHolder<HasSufficientBalanceToReconcileCallback>* holder,
    bool sufficient) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(sufficient);
  }
  delete holder;
}

void BatLedgerImpl::HasSufficientBalanceToReconcile(
    HasSufficientBalanceToReconcileCallback callback) {
  auto* holder = new CallbackHolder<HasSufficientBalanceToReconcileCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->HasSufficientBalanceToReconcile(
      std::bind(BatLedgerImpl::OnHasSufficientBalanceToReconcile, holder, _1));
}

void BatLedgerImpl::SetCatalogIssuers(const std::string& info) {
  ledger_->SetCatalogIssuers(info);
}

void BatLedgerImpl::ConfirmAd(const std::string& info) {
  ledger_->ConfirmAd(info);
}

void BatLedgerImpl::ConfirmAction(const std::string& uuid,
    const std::string& creative_set_id,
    const std::string& type) {
  ledger_->ConfirmAction(uuid, creative_set_id, type);
}

// static
void BatLedgerImpl::OnGetTransactionHistory(
    CallbackHolder<GetTransactionHistoryCallback>* holder,
    std::unique_ptr<ledger::TransactionsInfo> history) {
  std::string json_transactions = history.get() ? history->ToJson() : "";
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(json_transactions);
  delete holder;
}

void BatLedgerImpl::GetTransactionHistory(
    GetTransactionHistoryCallback callback) {
  auto* holder = new CallbackHolder<GetTransactionHistoryCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetTransactionHistory(
      std::bind(BatLedgerImpl::OnGetTransactionHistory,
          holder, _1));
}

void BatLedgerImpl::OnGetRewardsInternalsInfo(
    CallbackHolder<GetRewardsInternalsInfoCallback>* holder,
    ledger::RewardsInternalsInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(info));
  delete holder;
}

void BatLedgerImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  auto* holder = new CallbackHolder<GetRewardsInternalsInfoCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetRewardsInternalsInfo(
    std::bind(BatLedgerImpl::OnGetRewardsInternalsInfo, holder, _1));
}

// static
void BatLedgerImpl::OnSaveRecurringTip(
    CallbackHolder<SaveRecurringTipCallback>* holder,
    const ledger::Result result) {
  if (holder->is_valid())
    std::move(holder->get()).Run(result);

  delete holder;
}

void BatLedgerImpl::SaveRecurringTip(
    ledger::RecurringTipPtr info,
    SaveRecurringTipCallback callback) {
  // deleted in OnSaveRecurringTip
  auto* holder = new CallbackHolder<SaveRecurringTipCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->SaveRecurringTip(std::move(info), std::bind(
      BatLedgerImpl::OnSaveRecurringTip, holder, _1));
}

// static
void BatLedgerImpl::OnGetRecurringTips(
    CallbackHolder<GetRecurringTipsCallback>* holder,
    ledger::PublisherInfoList list,
    uint32_t num) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(list));

  delete holder;
}

void BatLedgerImpl::GetRecurringTips(GetRecurringTipsCallback callback) {
  auto* holder = new CallbackHolder<GetRecurringTipsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetRecurringTips(std::bind(
      BatLedgerImpl::OnGetRecurringTips, holder, _1, _2));
}

// static
void BatLedgerImpl::OnGetOneTimeTips(
    CallbackHolder<GetRecurringTipsCallback>* holder,
    ledger::PublisherInfoList list,
    uint32_t num) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(list));

  delete holder;
}

void BatLedgerImpl::GetOneTimeTips(GetOneTimeTipsCallback callback) {
  auto* holder = new CallbackHolder<GetOneTimeTipsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetOneTimeTips(std::bind(
      BatLedgerImpl::OnGetOneTimeTips, holder, _1, _2));
}

// static
void BatLedgerImpl::OnGetActivityInfoList(
    CallbackHolder<GetActivityInfoListCallback>* holder,
    ledger::PublisherInfoList list,
    uint32_t num) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(list), num);

  delete holder;
}

void BatLedgerImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::ActivityInfoFilterPtr filter,
    GetActivityInfoListCallback callback) {
  auto* holder = new CallbackHolder<GetActivityInfoListCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetActivityInfoList(
      start,
      limit,
      std::move(filter),
      std::bind(BatLedgerImpl::OnGetActivityInfoList, holder, _1, _2));
}

// static
void BatLedgerImpl::OnLoadPublisherInfo(
    CallbackHolder<LoadPublisherInfoCallback>* holder,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(publisher_info));

  delete holder;
}

void BatLedgerImpl::LoadPublisherInfo(
    const std::string& publisher_key,
    LoadPublisherInfoCallback callback) {
  auto* holder = new CallbackHolder<LoadPublisherInfoCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetPublisherInfo(
      publisher_key,
      std::bind(BatLedgerImpl::OnLoadPublisherInfo, holder, _1, _2));
}

// static
void BatLedgerImpl::OnSaveMediaInfoCallback(
    CallbackHolder<SaveMediaInfoCallback>* holder,
    ledger::Result result,
    ledger::PublisherInfoPtr publisher_info) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result, std::move(publisher_info));
  }

  delete holder;
}

void BatLedgerImpl::SaveMediaInfo(
    const std::string& type,
    const base::flat_map<std::string, std::string>& args,
    SaveMediaInfoCallback callback) {
  auto* holder = new CallbackHolder<SaveMediaInfoCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->SaveMediaInfo(
      type,
      base::FlatMapToMap(args),
      std::bind(BatLedgerImpl::OnSaveMediaInfoCallback, holder, _1, _2));
}

void BatLedgerImpl::OnRefreshPublisher(
    CallbackHolder<RefreshPublisherCallback>* holder,
    ledger::PublisherStatus status) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(status);

  delete holder;
}

void BatLedgerImpl::RefreshPublisher(
    const std::string& publisher_key,
    RefreshPublisherCallback callback) {
  auto* holder = new CallbackHolder<RefreshPublisherCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->RefreshPublisher(
      publisher_key,
      std::bind(BatLedgerImpl::OnRefreshPublisher, holder, _1));
}

void BatLedgerImpl::StartMonthlyContribution() {
  ledger_->StartMonthlyContribution();
}

void BatLedgerImpl::SetInlineTipSetting(const std::string& key, bool enabled) {
  ledger_->SetInlineTipSetting(key, enabled);
}

void BatLedgerImpl::GetInlineTipSetting(
    const std::string& key,
    GetInlineTipSettingCallback callback) {
  std::move(callback).Run(ledger_->GetInlineTipSetting(key));
}

void BatLedgerImpl::GetShareURL(
    const std::string& type,
    const base::flat_map<std::string, std::string>& args,
    GetShareURLCallback callback) {
  std::move(callback).Run(ledger_->GetShareURL(type, base::FlatMapToMap(args)));
}

// static
void BatLedgerImpl::OnGetPendingContributions(
    CallbackHolder<GetPendingContributionsCallback>* holder,
    ledger::PendingContributionInfoList list) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(list));
  }
  delete holder;
}

void BatLedgerImpl::GetPendingContributions(
    GetPendingContributionsCallback callback) {
  auto* holder = new CallbackHolder<GetPendingContributionsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetPendingContributions(std::bind(
      BatLedgerImpl::OnGetPendingContributions, holder, _1));
}

// static
void BatLedgerImpl::OnRemovePendingContribution(
    CallbackHolder<RemovePendingContributionCallback>* holder,
    ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void BatLedgerImpl::RemovePendingContribution(
    const std::string& publisher_key,
    const std::string& viewing_id,
    uint64_t added_date,
    RemovePendingContributionCallback callback) {
  auto* holder = new CallbackHolder<RemovePendingContributionCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->RemovePendingContribution(
      publisher_key,
      viewing_id,
      added_date,
      std::bind(BatLedgerImpl::OnRemovePendingContribution,
                holder,
                _1));
}

// static
void BatLedgerImpl::OnRemoveAllPendingContributions(
    CallbackHolder<RemovePendingContributionCallback>* holder,
    ledger::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void BatLedgerImpl::RemoveAllPendingContributions(
    RemovePendingContributionCallback callback) {
  auto* holder = new CallbackHolder<RemovePendingContributionCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->RemoveAllPendingContributions(
      std::bind(BatLedgerImpl::OnRemoveAllPendingContributions,
                holder,
                _1));
}

// static
void BatLedgerImpl::OnGetPendingContributionsTotal(
    CallbackHolder<GetPendingContributionsTotalCallback>* holder,
    double amount) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(amount);
  }
  delete holder;
}

void BatLedgerImpl::GetPendingContributionsTotal(
    GetPendingContributionsTotalCallback callback) {
  auto* holder = new CallbackHolder<GetPendingContributionsTotalCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetPendingContributionsTotal(
      std::bind(BatLedgerImpl::OnGetPendingContributionsTotal,
                holder,
                _1));
}

// static
void BatLedgerImpl::OnFetchBalance(
    CallbackHolder<FetchBalanceCallback>* holder,
    ledger::Result result,
    ledger::BalancePtr balance) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(balance));
  delete holder;
}

void BatLedgerImpl::FetchBalance(
    FetchBalanceCallback callback) {
  auto* holder = new CallbackHolder<FetchBalanceCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->FetchBalance(
      std::bind(BatLedgerImpl::OnFetchBalance,
                holder,
                _1,
                _2));
}

// static
void BatLedgerImpl::OnGetExternalWallet(
    CallbackHolder<GetExternalWalletCallback>* holder,
    ledger::Result result,
    ledger::ExternalWalletPtr wallet) {
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(wallet));
  delete holder;
}

void BatLedgerImpl::GetExternalWallet(const std::string& wallet_type,
                                      GetExternalWalletCallback callback) {
  auto* holder = new CallbackHolder<GetExternalWalletCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetExternalWallet(
      wallet_type,
      std::bind(BatLedgerImpl::OnGetExternalWallet,
                holder,
                _1,
                _2));
}

// static
void BatLedgerImpl::OnExternalWalletAuthorization(
    CallbackHolder<ExternalWalletAuthorizationCallback>* holder,
    ledger::Result result,
    const std::map<std::string, std::string>& args) {
  if (holder->is_valid())
    std::move(holder->get()).Run(result, base::MapToFlatMap(args));
  delete holder;
}

void BatLedgerImpl::ExternalWalletAuthorization(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ExternalWalletAuthorizationCallback callback) {
  auto* holder = new CallbackHolder<ExternalWalletAuthorizationCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->ExternalWalletAuthorization(
      wallet_type,
      base::FlatMapToMap(args),
      std::bind(BatLedgerImpl::OnExternalWalletAuthorization,
                holder,
                _1,
                _2));
}

// static
void BatLedgerImpl::OnDisconnectWallet(
    CallbackHolder<DisconnectWalletCallback>* holder,
    ledger::Result result) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void BatLedgerImpl::DisconnectWallet(
    const std::string& wallet_type,
    DisconnectWalletCallback callback) {
  auto* holder = new CallbackHolder<DisconnectWalletCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->DisconnectWallet(
      wallet_type,
      std::bind(BatLedgerImpl::OnDisconnectWallet,
                holder,
                _1));
}

// static
void BatLedgerImpl::OnGetAnonWalletStatus(
    CallbackHolder<GetAnonWalletStatusCallback>* holder,
    const ledger::Result result) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void BatLedgerImpl::GetAnonWalletStatus(GetAnonWalletStatusCallback callback) {
  auto* holder = new CallbackHolder<GetAnonWalletStatusCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetAnonWalletStatus(
      std::bind(BatLedgerImpl::OnGetAnonWalletStatus,
                holder,
                _1));
}

}  // namespace bat_ledger
