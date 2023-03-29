/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_impl.h"

#include "brave/components/brave_rewards/core/common/legacy_callback_helpers.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

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
    mojo::PendingAssociatedRemote<mojom::RewardsService> rewards_service,
    CreateLedgerCallback callback) {
  ledger_ = std::make_unique<ledger::LedgerImpl>(std::move(rewards_service));

  std::move(callback).Run();
}

void RewardsUtilityServiceImpl::InitializeLedger(
    bool execute_create_script,
    InitializeLedgerCallback callback) {
  ledger_->Initialize(execute_create_script,
                      ledger::ToLegacyCallback(std::move(callback)));
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

void RewardsUtilityServiceImpl::SetReconcileInterval(int32_t interval) {
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

void RewardsUtilityServiceImpl::GetBalanceReport(
    ledger::mojom::ActivityMonth month,
    int32_t year,
    GetBalanceReportCallback callback) {
  ledger_->GetBalanceReport(month, year,
                            ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetPublisherActivityFromUrl(
    uint64_t window_id,
    ledger::mojom::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  ledger_->GetPublisherActivityFromUrl(window_id, std::move(visit_data),
                                       publisher_blob);
}

void RewardsUtilityServiceImpl::GetAutoContributionAmount(
    GetAutoContributionAmountCallback callback) {
  std::move(callback).Run(ledger_->GetAutoContributionAmount());
}

void RewardsUtilityServiceImpl::GetPublisherBanner(
    const std::string& publisher_id,
    GetPublisherBannerCallback callback) {
  ledger_->GetPublisherBanner(publisher_id,
                              ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::OneTimeTip(const std::string& publisher_key,
                                           double amount,
                                           OneTimeTipCallback callback) {
  ledger_->OneTimeTip(publisher_key, amount,
                      ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::RemoveRecurringTip(
    const std::string& publisher_key,
    RemoveRecurringTipCallback callback) {
  ledger_->RemoveRecurringTip(publisher_key,
                              ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetCreationStamp(
    GetCreationStampCallback callback) {
  std::move(callback).Run(ledger_->GetCreationStamp());
}

void RewardsUtilityServiceImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  ledger_->GetRewardsInternalsInfo(
      ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::SaveRecurringTip(
    ledger::mojom::RecurringTipPtr info,
    SaveRecurringTipCallback callback) {
  ledger_->SaveRecurringTip(std::move(info),
                            ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::SendContribution(
    const std::string& publisher_id,
    double amount,
    bool set_monthly,
    SendContributionCallback callback) {
  ledger_->SendContribution(publisher_id, amount, set_monthly,
                            std::move(callback));
}

void RewardsUtilityServiceImpl::GetRecurringTips(
    GetRecurringTipsCallback callback) {
  ledger_->GetRecurringTips(ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetOneTimeTips(
    GetOneTimeTipsCallback callback) {
  ledger_->GetOneTimeTips(ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::mojom::ActivityInfoFilterPtr filter,
    GetActivityInfoListCallback callback) {
  ledger_->GetActivityInfoList(start, limit, std::move(filter),
                               ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetPublishersVisitedCount(
    GetPublishersVisitedCountCallback callback) {
  ledger_->GetPublishersVisitedCount(std::move(callback));
}

void RewardsUtilityServiceImpl::GetExcludedList(
    GetExcludedListCallback callback) {
  ledger_->GetExcludedList(ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::RefreshPublisher(
    const std::string& publisher_key,
    RefreshPublisherCallback callback) {
  ledger_->RefreshPublisher(publisher_key,
                            ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::StartContributionsForTesting() {
  ledger_->StartContributionsForTesting();  // IN-TEST
}

void RewardsUtilityServiceImpl::UpdateMediaDuration(
    uint64_t window_id,
    const std::string& publisher_key,
    uint64_t duration,
    bool first_visit) {
  ledger_->UpdateMediaDuration(window_id, publisher_key, duration, first_visit);
}

void RewardsUtilityServiceImpl::IsPublisherRegistered(
    const std::string& publisher_id,
    IsPublisherRegisteredCallback callback) {
  ledger_->IsPublisherRegistered(publisher_id,
                                 ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetPublisherInfo(
    const std::string& publisher_key,
    GetPublisherInfoCallback callback) {
  ledger_->GetPublisherInfo(publisher_key,
                            ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetPublisherPanelInfo(
    const std::string& publisher_key,
    GetPublisherPanelInfoCallback callback) {
  ledger_->GetPublisherPanelInfo(publisher_key,
                                 ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::SavePublisherInfo(
    uint64_t window_id,
    ledger::mojom::PublisherInfoPtr publisher_info,
    SavePublisherInfoCallback callback) {
  ledger_->SavePublisherInfo(window_id, std::move(publisher_info),
                             ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::SetInlineTippingPlatformEnabled(
    ledger::mojom::InlineTipsPlatforms platform,
    bool enabled) {
  ledger_->SetInlineTippingPlatformEnabled(platform, enabled);
}

void RewardsUtilityServiceImpl::GetInlineTippingPlatformEnabled(
    ledger::mojom::InlineTipsPlatforms platform,
    GetInlineTippingPlatformEnabledCallback callback) {
  std::move(callback).Run(ledger_->GetInlineTippingPlatformEnabled(platform));
}

void RewardsUtilityServiceImpl::GetShareURL(
    const base::flat_map<std::string, std::string>& args,
    GetShareURLCallback callback) {
  std::move(callback).Run(ledger_->GetShareURL(args));
}

void RewardsUtilityServiceImpl::GetPendingContributions(
    GetPendingContributionsCallback callback) {
  ledger_->GetPendingContributions(
      ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::RemovePendingContribution(
    uint64_t id,
    RemovePendingContributionCallback callback) {
  ledger_->RemovePendingContribution(
      id, ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::RemoveAllPendingContributions(
    RemovePendingContributionCallback callback) {
  ledger_->RemoveAllPendingContributions(
      ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetPendingContributionsTotal(
    GetPendingContributionsTotalCallback callback) {
  ledger_->GetPendingContributionsTotal(
      ledger::ToLegacyCallback(std::move(callback)));
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

void RewardsUtilityServiceImpl::GetTransactionReport(
    ledger::mojom::ActivityMonth month,
    int year,
    GetTransactionReportCallback callback) {
  ledger_->GetTransactionReport(month, year,
                                ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetContributionReport(
    ledger::mojom::ActivityMonth month,
    int year,
    GetContributionReportCallback callback) {
  ledger_->GetContributionReport(month, year,
                                 ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetAllContributions(
    GetAllContributionsCallback callback) {
  ledger_->GetAllContributions(ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::SavePublisherInfoForTip(
    ledger::mojom::PublisherInfoPtr info,
    SavePublisherInfoForTipCallback callback) {
  ledger_->SavePublisherInfoForTip(
      std::move(info), ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetMonthlyReport(
    ledger::mojom::ActivityMonth month,
    int year,
    GetMonthlyReportCallback callback) {
  ledger_->GetMonthlyReport(month, year,
                            ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetAllMonthlyReportIds(
    GetAllMonthlyReportIdsCallback callback) {
  ledger_->GetAllMonthlyReportIds(
      ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetAllPromotions(
    GetAllPromotionsCallback callback) {
  ledger_->GetAllPromotions(ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::Shutdown(ShutdownCallback callback) {
  ledger_->Shutdown(ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetEventLogs(GetEventLogsCallback callback) {
  ledger_->GetEventLogs(ledger::ToLegacyCallback(std::move(callback)));
}

void RewardsUtilityServiceImpl::GetRewardsWallet(
    GetRewardsWalletCallback callback) {
  ledger_->GetRewardsWallet(ledger::ToLegacyCallback(std::move(callback)));
}

}  // namespace rewards
