/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_impl.h"

#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/services/bat_ledger/bat_ledger_client_mojo_bridge.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace {

bool testing() {
  return ledger::is_testing;
}

}  // namespace

namespace rewards {

RewardsUtilityServiceImpl::RewardsUtilityServiceImpl(
    mojo::PendingReceiver<mojom::RewardsUtilityService> pending_receiver)
    : utility_service_receiver_(this, std::move(pending_receiver)) {}

RewardsUtilityServiceImpl::~RewardsUtilityServiceImpl() = default;

void RewardsUtilityServiceImpl::CreateLedger(
    mojo::PendingAssociatedRemote<mojom::BatLedgerClient> client_info,
    CreateLedgerCallback callback) {
  ledger_ = std::make_unique<ledger::LedgerImpl>(
      std::make_unique<BatLedgerClientMojoBridge>(std::move(client_info)));

  std::move(callback).Run();
}

void RewardsUtilityServiceImpl::SetEnvironment(
    ledger::mojom::Environment environment) {
  DCHECK(!ledger_ || testing());
  ledger::_environment = environment;
}

void RewardsUtilityServiceImpl::SetDebug(bool is_debug) {
  DCHECK(!ledger_ || testing());
  ledger::is_debug = is_debug;
}

void RewardsUtilityServiceImpl::SetReconcileInterval(const int32_t interval) {
  DCHECK(!ledger_ || testing());
  ledger::reconcile_interval = interval;
}

void RewardsUtilityServiceImpl::SetRetryInterval(int32_t interval) {
  DCHECK(!ledger_ || testing());
  ledger::retry_interval = interval;
}

void RewardsUtilityServiceImpl::SetTesting() {
  ledger::is_testing = true;
}

void RewardsUtilityServiceImpl::SetStateMigrationTargetVersionForTesting(
    int32_t version) {
  ledger::state_migration_target_version_for_testing = version;
}

void RewardsUtilityServiceImpl::GetEnvironment(
    GetEnvironmentCallback callback) {
  std::move(callback).Run(ledger::_environment);
}

void RewardsUtilityServiceImpl::GetDebug(GetDebugCallback callback) {
  std::move(callback).Run(ledger::is_debug);
}

void RewardsUtilityServiceImpl::GetReconcileInterval(
    GetReconcileIntervalCallback callback) {
  std::move(callback).Run(ledger::reconcile_interval);
}

void RewardsUtilityServiceImpl::GetRetryInterval(
    GetRetryIntervalCallback callback) {
  std::move(callback).Run(ledger::retry_interval);
}

void RewardsUtilityServiceImpl::OnInitializeLedger(
    CallbackHolder<InitializeLedgerCallback>* holder,
    ledger::mojom::Result result) {
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}
void RewardsUtilityServiceImpl::InitializeLedger(
    const bool execute_create_script,
    InitializeLedgerCallback callback) {
  auto* holder = new CallbackHolder<InitializeLedgerCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->Initialize(
      execute_create_script,
      std::bind(RewardsUtilityServiceImpl::OnInitializeLedger, holder, _1));
}

void RewardsUtilityServiceImpl::CreateRewardsWallet(
    const std::string& country,
    CreateRewardsWalletCallback callback) {
  ledger_->CreateRewardsWallet(country, std::move(callback));
}

void RewardsUtilityServiceImpl::GetRewardsParameters(
    GetRewardsParametersCallback callback) {
  ledger_->GetRewardsParameters(std::move(callback));
}

void RewardsUtilityServiceImpl::GetAutoContributeProperties(
    GetAutoContributePropertiesCallback callback) {
  ledger::mojom::AutoContributePropertiesPtr props =
      ledger_->GetAutoContributeProperties();
  if (!props) {
    props = ledger::mojom::AutoContributeProperties::New();
  }
  std::move(callback).Run(std::move(props));
}

void RewardsUtilityServiceImpl::GetPublisherMinVisitTime(
    GetPublisherMinVisitTimeCallback callback) {
  std::move(callback).Run(ledger_->GetPublisherMinVisitTime());
}

void RewardsUtilityServiceImpl::GetPublisherMinVisits(
    GetPublisherMinVisitsCallback callback) {
  std::move(callback).Run(ledger_->GetPublisherMinVisits());
}

void RewardsUtilityServiceImpl::GetPublisherAllowNonVerified(
    GetPublisherAllowNonVerifiedCallback callback) {
  std::move(callback).Run(ledger_->GetPublisherAllowNonVerified());
}

void RewardsUtilityServiceImpl::GetAutoContributeEnabled(
    GetAutoContributeEnabledCallback callback) {
  std::move(callback).Run(ledger_->GetAutoContributeEnabled());
}

void RewardsUtilityServiceImpl::GetReconcileStamp(
    GetReconcileStampCallback callback) {
  std::move(callback).Run(ledger_->GetReconcileStamp());
}

void RewardsUtilityServiceImpl::OnLoad(ledger::mojom::VisitDataPtr visit_data,
                                       uint64_t current_time) {
  ledger_->OnLoad(std::move(visit_data), current_time);
}

void RewardsUtilityServiceImpl::OnUnload(uint32_t tab_id,
                                         uint64_t current_time) {
  ledger_->OnUnload(tab_id, current_time);
}

void RewardsUtilityServiceImpl::OnShow(uint32_t tab_id, uint64_t current_time) {
  ledger_->OnShow(tab_id, current_time);
}

void RewardsUtilityServiceImpl::OnHide(uint32_t tab_id, uint64_t current_time) {
  ledger_->OnHide(tab_id, current_time);
}

void RewardsUtilityServiceImpl::OnForeground(uint32_t tab_id,
                                             uint64_t current_time) {
  ledger_->OnForeground(tab_id, current_time);
}

void RewardsUtilityServiceImpl::OnBackground(uint32_t tab_id,
                                             uint64_t current_time) {
  ledger_->OnBackground(tab_id, current_time);
}

void RewardsUtilityServiceImpl::OnXHRLoad(
    uint32_t tab_id,
    const std::string& url,
    const base::flat_map<std::string, std::string>& parts,
    const std::string& first_party_url,
    const std::string& referrer,
    ledger::mojom::VisitDataPtr visit_data) {
  ledger_->OnXHRLoad(tab_id, url, parts, first_party_url, referrer,
                     std::move(visit_data));
}

void RewardsUtilityServiceImpl::SetPublisherExclude(
    const std::string& publisher_key,
    ledger::mojom::PublisherExclude exclude,
    SetPublisherExcludeCallback callback) {
  ledger_->SetPublisherExclude(publisher_key, exclude, std::move(callback));
}

void RewardsUtilityServiceImpl::RestorePublishers(
    RestorePublishersCallback callback) {
  ledger_->RestorePublishers(std::move(callback));
}

void RewardsUtilityServiceImpl::FetchPromotions(
    FetchPromotionsCallback callback) {
  ledger_->FetchPromotions(std::move(callback));
}

void RewardsUtilityServiceImpl::ClaimPromotion(
    const std::string& promotion_id,
    const std::string& payload,
    ClaimPromotionCallback callback) {
  ledger_->ClaimPromotion(promotion_id, payload, std::move(callback));
}

void RewardsUtilityServiceImpl::AttestPromotion(
    const std::string& promotion_id,
    const std::string& solution,
    AttestPromotionCallback callback) {
  ledger_->AttestPromotion(promotion_id, solution, std::move(callback));
}

void RewardsUtilityServiceImpl::SetPublisherMinVisitTime(
    int duration_in_seconds) {
  ledger_->SetPublisherMinVisitTime(duration_in_seconds);
}

void RewardsUtilityServiceImpl::SetPublisherMinVisits(int visits) {
  ledger_->SetPublisherMinVisits(visits);
}

void RewardsUtilityServiceImpl::SetPublisherAllowNonVerified(bool allow) {
  ledger_->SetPublisherAllowNonVerified(allow);
}

void RewardsUtilityServiceImpl::SetAutoContributionAmount(double amount) {
  ledger_->SetAutoContributionAmount(amount);
}

void RewardsUtilityServiceImpl::SetAutoContributeEnabled(bool enabled) {
  ledger_->SetAutoContributeEnabled(enabled);
}

// static
void RewardsUtilityServiceImpl::OnGetBalanceReport(
    CallbackHolder<GetBalanceReportCallback>* holder,
    const ledger::mojom::Result result,
    ledger::mojom::BalanceReportInfoPtr report_info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(report_info));
  delete holder;
}
void RewardsUtilityServiceImpl::GetBalanceReport(
    const ledger::mojom::ActivityMonth month,
    const int32_t year,
    GetBalanceReportCallback callback) {
  auto* holder = new CallbackHolder<GetBalanceReportCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->GetBalanceReport(
      month, year,
      std::bind(RewardsUtilityServiceImpl::OnGetBalanceReport, holder, _1, _2));
}

void RewardsUtilityServiceImpl::GetPublisherActivityFromUrl(
    uint64_t window_id,
    ledger::mojom::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  ledger_->GetPublisherActivityFromUrl(
      window_id, std::move(visit_data), publisher_blob);
}

// static
void RewardsUtilityServiceImpl::OnGetPublisherBanner(
    CallbackHolder<GetPublisherBannerCallback>* holder,
    ledger::mojom::PublisherBannerPtr banner) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(banner));
  delete holder;
}

void RewardsUtilityServiceImpl::GetPublisherBanner(
    const std::string& publisher_id,
    GetPublisherBannerCallback callback) {
  // delete in OnGetPublisherBanner
  auto* holder = new CallbackHolder<GetPublisherBannerCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->GetPublisherBanner(
      publisher_id,
      std::bind(RewardsUtilityServiceImpl::OnGetPublisherBanner, holder, _1));
}

void RewardsUtilityServiceImpl::GetAutoContributionAmount(
    GetAutoContributionAmountCallback callback) {
  std::move(callback).Run(ledger_->GetAutoContributionAmount());
}

void RewardsUtilityServiceImpl::OnOneTimeTip(
    CallbackHolder<OneTimeTipCallback>* holder,
    const ledger::mojom::Result result) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result);
  delete holder;
}

void RewardsUtilityServiceImpl::OneTimeTip(const std::string& publisher_key,
                                           const double amount,
                                           OneTimeTipCallback callback) {
  // deleted in OnOneTimeTip
  auto* holder = new CallbackHolder<OneTimeTipCallback>(
    AsWeakPtr(), std::move(callback));
  ledger_->OneTimeTip(
      publisher_key, amount,
      std::bind(RewardsUtilityServiceImpl::OnOneTimeTip, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnRemoveRecurringTip(
    CallbackHolder<RemoveRecurringTipCallback>* holder,
    const ledger::mojom::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void RewardsUtilityServiceImpl::RemoveRecurringTip(
    const std::string& publisher_key,
    RemoveRecurringTipCallback callback) {
  auto* holder = new CallbackHolder<RemoveRecurringTipCallback>(
            AsWeakPtr(), std::move(callback));
  ledger_->RemoveRecurringTip(
      publisher_key,
      std::bind(RewardsUtilityServiceImpl::OnRemoveRecurringTip, holder, _1));
}

void RewardsUtilityServiceImpl::GetCreationStamp(
    GetCreationStampCallback callback) {
  std::move(callback).Run(ledger_->GetCreationStamp());
}

void RewardsUtilityServiceImpl::OnGetRewardsInternalsInfo(
    CallbackHolder<GetRewardsInternalsInfoCallback>* holder,
    ledger::mojom::RewardsInternalsInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(info));
  delete holder;
}

void RewardsUtilityServiceImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  auto* holder = new CallbackHolder<GetRewardsInternalsInfoCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetRewardsInternalsInfo(std::bind(
      RewardsUtilityServiceImpl::OnGetRewardsInternalsInfo, holder, _1));
}

void RewardsUtilityServiceImpl::SendContribution(
    const std::string& publisher_id,
    double amount,
    bool set_monthly,
    SendContributionCallback callback) {
  ledger_->SendContribution(publisher_id, amount, set_monthly,
                            std::move(callback));
}

// static
void RewardsUtilityServiceImpl::OnSaveRecurringTip(
    CallbackHolder<SaveRecurringTipCallback>* holder,
    const ledger::mojom::Result result) {
  if (holder->is_valid())
    std::move(holder->get()).Run(result);

  delete holder;
}

void RewardsUtilityServiceImpl::SaveRecurringTip(
    ledger::mojom::RecurringTipPtr info,
    SaveRecurringTipCallback callback) {
  // deleted in OnSaveRecurringTip
  auto* holder = new CallbackHolder<SaveRecurringTipCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->SaveRecurringTip(
      std::move(info),
      std::bind(RewardsUtilityServiceImpl::OnSaveRecurringTip, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnGetRecurringTips(
    CallbackHolder<GetRecurringTipsCallback>* holder,
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(list));

  delete holder;
}

void RewardsUtilityServiceImpl::GetRecurringTips(
    GetRecurringTipsCallback callback) {
  auto* holder = new CallbackHolder<GetRecurringTipsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetRecurringTips(
      std::bind(RewardsUtilityServiceImpl::OnGetRecurringTips, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnGetOneTimeTips(
    CallbackHolder<GetRecurringTipsCallback>* holder,
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(list));

  delete holder;
}

void RewardsUtilityServiceImpl::GetOneTimeTips(
    GetOneTimeTipsCallback callback) {
  auto* holder = new CallbackHolder<GetOneTimeTipsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetOneTimeTips(
      std::bind(RewardsUtilityServiceImpl::OnGetOneTimeTips, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnGetActivityInfoList(
    CallbackHolder<GetActivityInfoListCallback>* holder,
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(list));

  delete holder;
}

void RewardsUtilityServiceImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::mojom::ActivityInfoFilterPtr filter,
    GetActivityInfoListCallback callback) {
  auto* holder = new CallbackHolder<GetActivityInfoListCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetActivityInfoList(
      start, limit, std::move(filter),
      std::bind(RewardsUtilityServiceImpl::OnGetActivityInfoList, holder, _1));
}

void RewardsUtilityServiceImpl::GetPublishersVisitedCount(
    GetPublishersVisitedCountCallback callback) {
  ledger_->GetPublishersVisitedCount(std::move(callback));
}

// static
void RewardsUtilityServiceImpl::OnGetExcludedList(
    CallbackHolder<GetExcludedListCallback>* holder,
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(list));

  delete holder;
}

void RewardsUtilityServiceImpl::GetExcludedList(
    GetExcludedListCallback callback) {
  auto* holder = new CallbackHolder<GetExcludedListCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetExcludedList(
      std::bind(RewardsUtilityServiceImpl::OnGetExcludedList, holder, _1));
}

void RewardsUtilityServiceImpl::UpdateMediaDuration(
    const uint64_t window_id,
    const std::string& publisher_key,
    const uint64_t duration,
    const bool first_visit) {
  ledger_->UpdateMediaDuration(window_id, publisher_key, duration, first_visit);
}

// static
void RewardsUtilityServiceImpl::OnIsPublisherRegistered(
    CallbackHolder<IsPublisherRegisteredCallback>* holder,
    bool is_registered) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(is_registered);
  delete holder;
}

void RewardsUtilityServiceImpl::IsPublisherRegistered(
    const std::string& publisher_id,
    IsPublisherRegisteredCallback callback) {
  auto* holder = new CallbackHolder<IsPublisherRegisteredCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->IsPublisherRegistered(
      publisher_id,
      std::bind(RewardsUtilityServiceImpl::OnIsPublisherRegistered, holder,
                _1));
}

// static
void RewardsUtilityServiceImpl::OnPublisherInfo(
    CallbackHolder<GetPublisherInfoCallback>* holder,
    const ledger::mojom::Result result,
    ledger::mojom::PublisherInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(info));
  delete holder;
}

void RewardsUtilityServiceImpl::GetPublisherInfo(
    const std::string& publisher_key,
    GetPublisherInfoCallback callback) {
  // deleted in OnPublisherInfo
  auto* holder = new CallbackHolder<GetPublisherInfoCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->GetPublisherInfo(
      publisher_key,
      std::bind(RewardsUtilityServiceImpl::OnPublisherInfo, holder, _1, _2));
}

// static
void RewardsUtilityServiceImpl::OnPublisherPanelInfo(
    CallbackHolder<GetPublisherPanelInfoCallback>* holder,
    const ledger::mojom::Result result,
    ledger::mojom::PublisherInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(info));
  delete holder;
}

void RewardsUtilityServiceImpl::GetPublisherPanelInfo(
    const std::string& publisher_key,
    GetPublisherPanelInfoCallback callback) {
  // deleted in OnPublisherPanelInfo
  auto* holder = new CallbackHolder<GetPublisherPanelInfoCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->GetPublisherPanelInfo(
      publisher_key, std::bind(RewardsUtilityServiceImpl::OnPublisherPanelInfo,
                               holder, _1, _2));
}

// static
void RewardsUtilityServiceImpl::OnSavePublisherInfo(
    CallbackHolder<SavePublisherInfoCallback>* holder,
    const ledger::mojom::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }

  delete holder;
}

void RewardsUtilityServiceImpl::SavePublisherInfo(
    const uint64_t window_id,
    ledger::mojom::PublisherInfoPtr publisher_info,
    SavePublisherInfoCallback callback) {
  auto* holder = new CallbackHolder<SavePublisherInfoCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->SavePublisherInfo(
      window_id, std::move(publisher_info),
      std::bind(RewardsUtilityServiceImpl::OnSavePublisherInfo, holder, _1));
}

void RewardsUtilityServiceImpl::OnRefreshPublisher(
    CallbackHolder<RefreshPublisherCallback>* holder,
    ledger::mojom::PublisherStatus status) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(status);

  delete holder;
}

void RewardsUtilityServiceImpl::RefreshPublisher(
    const std::string& publisher_key,
    RefreshPublisherCallback callback) {
  auto* holder = new CallbackHolder<RefreshPublisherCallback>(
      AsWeakPtr(), std::move(callback));
  ledger_->RefreshPublisher(
      publisher_key,
      std::bind(RewardsUtilityServiceImpl::OnRefreshPublisher, holder, _1));
}

void RewardsUtilityServiceImpl::StartContributionsForTesting() {
  ledger_->StartContributionsForTesting();  // IN-TEST
}

void RewardsUtilityServiceImpl::SetInlineTippingPlatformEnabled(
    const ledger::mojom::InlineTipsPlatforms platform,
    bool enabled) {
  ledger_->SetInlineTippingPlatformEnabled(platform, enabled);
}

void RewardsUtilityServiceImpl::GetInlineTippingPlatformEnabled(
    const ledger::mojom::InlineTipsPlatforms platform,
    GetInlineTippingPlatformEnabledCallback callback) {
  std::move(callback).Run(ledger_->GetInlineTippingPlatformEnabled(platform));
}

void RewardsUtilityServiceImpl::GetShareURL(
    const base::flat_map<std::string, std::string>& args,
    GetShareURLCallback callback) {
  std::move(callback).Run(ledger_->GetShareURL(args));
}

// static
void RewardsUtilityServiceImpl::OnGetPendingContributions(
    CallbackHolder<GetPendingContributionsCallback>* holder,
    std::vector<ledger::mojom::PendingContributionInfoPtr> list) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(list));
  }
  delete holder;
}

void RewardsUtilityServiceImpl::GetPendingContributions(
    GetPendingContributionsCallback callback) {
  auto* holder = new CallbackHolder<GetPendingContributionsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetPendingContributions(std::bind(
      RewardsUtilityServiceImpl::OnGetPendingContributions, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnRemovePendingContribution(
    CallbackHolder<RemovePendingContributionCallback>* holder,
    ledger::mojom::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void RewardsUtilityServiceImpl::RemovePendingContribution(
    const uint64_t id,
    RemovePendingContributionCallback callback) {
  auto* holder = new CallbackHolder<RemovePendingContributionCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->RemovePendingContribution(
      id, std::bind(RewardsUtilityServiceImpl::OnRemovePendingContribution,
                    holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnRemoveAllPendingContributions(
    CallbackHolder<RemovePendingContributionCallback>* holder,
    ledger::mojom::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }
  delete holder;
}

void RewardsUtilityServiceImpl::RemoveAllPendingContributions(
    RemovePendingContributionCallback callback) {
  auto* holder = new CallbackHolder<RemovePendingContributionCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->RemoveAllPendingContributions(std::bind(
      RewardsUtilityServiceImpl::OnRemoveAllPendingContributions, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnGetPendingContributionsTotal(
    CallbackHolder<GetPendingContributionsTotalCallback>* holder,
    double amount) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(amount);
  }
  delete holder;
}

void RewardsUtilityServiceImpl::GetPendingContributionsTotal(
    GetPendingContributionsTotalCallback callback) {
  auto* holder = new CallbackHolder<GetPendingContributionsTotalCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetPendingContributionsTotal(std::bind(
      RewardsUtilityServiceImpl::OnGetPendingContributionsTotal, holder, _1));
}

void RewardsUtilityServiceImpl::FetchBalance(FetchBalanceCallback callback) {
  ledger_->FetchBalance(std::move(callback));
}

void RewardsUtilityServiceImpl::GetExternalWallet(
    const std::string& wallet_type,
    GetExternalWalletCallback callback) {
  ledger_->GetExternalWallet(wallet_type, std::move(callback));
}

void RewardsUtilityServiceImpl::ConnectExternalWallet(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ConnectExternalWalletCallback callback) {
  ledger_->ConnectExternalWallet(wallet_type, args, std::move(callback));
}

// static
void RewardsUtilityServiceImpl::OnGetTransactionReport(
    CallbackHolder<GetTransactionReportCallback>* holder,
    std::vector<ledger::mojom::TransactionReportInfoPtr> list) {
  if (!holder) {
    return;
  }

  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(list));
  }
  delete holder;
}

void RewardsUtilityServiceImpl::GetTransactionReport(
    const ledger::mojom::ActivityMonth month,
    const int year,
    GetTransactionReportCallback callback) {
  auto* holder = new CallbackHolder<GetTransactionReportCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetTransactionReport(
      month, year,
      std::bind(RewardsUtilityServiceImpl::OnGetTransactionReport, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnGetContributionReport(
    CallbackHolder<GetContributionReportCallback>* holder,
    std::vector<ledger::mojom::ContributionReportInfoPtr> list) {
  if (!holder) {
    return;
  }

  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(list));
  }
  delete holder;
}

void RewardsUtilityServiceImpl::GetContributionReport(
    const ledger::mojom::ActivityMonth month,
    const int year,
    GetContributionReportCallback callback) {
  auto* holder = new CallbackHolder<GetContributionReportCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetContributionReport(
      month, year,
      std::bind(RewardsUtilityServiceImpl::OnGetContributionReport, holder,
                _1));
}

// static
void RewardsUtilityServiceImpl::OnGetAllContributions(
    CallbackHolder<GetAllContributionsCallback>* holder,
    std::vector<ledger::mojom::ContributionInfoPtr> list) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(std::move(list));

  delete holder;
}

void RewardsUtilityServiceImpl::GetAllContributions(
    GetAllContributionsCallback callback) {
  auto* holder = new CallbackHolder<GetAllContributionsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetAllContributions(
      std::bind(RewardsUtilityServiceImpl::OnGetAllContributions, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnSavePublisherInfoForTip(
    CallbackHolder<SavePublisherInfoForTipCallback>* holder,
    const ledger::mojom::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }

  delete holder;
}

void RewardsUtilityServiceImpl::SavePublisherInfoForTip(
    ledger::mojom::PublisherInfoPtr info,
    SavePublisherInfoForTipCallback callback) {
  auto* holder = new CallbackHolder<SavePublisherInfoForTipCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->SavePublisherInfoForTip(
      std::move(info),
      std::bind(RewardsUtilityServiceImpl::OnSavePublisherInfoForTip, holder,
                _1));
}

// static
void RewardsUtilityServiceImpl::OnGetMonthlyReport(
    CallbackHolder<GetMonthlyReportCallback>* holder,
    const ledger::mojom::Result result,
    ledger::mojom::MonthlyReportInfoPtr info) {
  DCHECK(holder);
  if (holder->is_valid())
    std::move(holder->get()).Run(result, std::move(info));

  delete holder;
}

void RewardsUtilityServiceImpl::GetMonthlyReport(
    const ledger::mojom::ActivityMonth month,
    const int year,
    GetMonthlyReportCallback callback) {
  auto* holder = new CallbackHolder<GetMonthlyReportCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetMonthlyReport(
      month, year,
      std::bind(RewardsUtilityServiceImpl::OnGetMonthlyReport, holder, _1, _2));
}

// static
void RewardsUtilityServiceImpl::OnGetAllMonthlyReportIds(
    CallbackHolder<GetAllMonthlyReportIdsCallback>* holder,
    const std::vector<std::string>& ids) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(ids);
  }

  delete holder;
}

void RewardsUtilityServiceImpl::GetAllMonthlyReportIds(
    GetAllMonthlyReportIdsCallback callback) {
  auto* holder = new CallbackHolder<GetAllMonthlyReportIdsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetAllMonthlyReportIds(std::bind(
      RewardsUtilityServiceImpl::OnGetAllMonthlyReportIds, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnGetAllPromotions(
    CallbackHolder<GetAllPromotionsCallback>* holder,
    base::flat_map<std::string, ledger::mojom::PromotionPtr> items) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(items));
  }

  delete holder;
}

void RewardsUtilityServiceImpl::GetAllPromotions(
    GetAllPromotionsCallback callback) {
  auto* holder = new CallbackHolder<GetAllPromotionsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetAllPromotions(
      std::bind(RewardsUtilityServiceImpl::OnGetAllPromotions, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnShutdown(
    CallbackHolder<ShutdownCallback>* holder,
    const ledger::mojom::Result result) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(result);
  }

  delete holder;
}

void RewardsUtilityServiceImpl::Shutdown(ShutdownCallback callback) {
  auto* holder = new CallbackHolder<ShutdownCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->Shutdown(
      std::bind(RewardsUtilityServiceImpl::OnShutdown, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnGetEventLogs(
    CallbackHolder<GetEventLogsCallback>* holder,
    std::vector<ledger::mojom::EventLogPtr> logs) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(logs));
  }

  delete holder;
}

void RewardsUtilityServiceImpl::GetEventLogs(GetEventLogsCallback callback) {
  auto* holder = new CallbackHolder<GetEventLogsCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetEventLogs(
      std::bind(RewardsUtilityServiceImpl::OnGetEventLogs, holder, _1));
}

// static
void RewardsUtilityServiceImpl::OnGetRewardsWallet(
    CallbackHolder<GetRewardsWalletCallback>* holder,
    ledger::mojom::RewardsWalletPtr wallet) {
  DCHECK(holder);
  if (holder->is_valid()) {
    std::move(holder->get()).Run(std::move(wallet));
  }

  delete holder;
}

void RewardsUtilityServiceImpl::GetRewardsWallet(
    GetRewardsWalletCallback callback) {
  auto* holder = new CallbackHolder<GetRewardsWalletCallback>(
      AsWeakPtr(), std::move(callback));

  ledger_->GetRewardsWallet(
      std::bind(RewardsUtilityServiceImpl::OnGetRewardsWallet, holder, _1));
}

}  // namespace rewards
