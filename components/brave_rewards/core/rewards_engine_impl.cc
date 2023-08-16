/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

#include <memory>
#include <vector>

#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/components/brave_rewards/core/common/legacy_callback_helpers.h"
#include "brave/components/brave_rewards/core/common/security_util.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/legacy/static_values.h"
#include "brave/components/brave_rewards/core/publisher/publisher_status_helper.h"

using std::placeholders::_1;

namespace brave_rewards::internal {

RewardsEngineImpl::RewardsEngineImpl(
    mojo::PendingAssociatedRemote<mojom::RewardsEngineClient> client_remote,
    ResultCallback callback)
    : client_(std::move(client_remote)),
      promotion_(*this),
      publisher_(*this),
      media_(*this),
      contribution_(*this),
      wallet_(*this),
      database_(*this),
      report_(*this),
      state_(*this),
      api_(*this),
      recovery_(*this),
      bitflyer_(*this),
      gemini_(*this),
      uphold_(*this),
      zebpay_(*this) {
  DCHECK(base::ThreadPoolInstance::Get());
  set_client_for_logging(client_.get());

  InitializeDatabase(std::move(callback));
}

RewardsEngineImpl::~RewardsEngineImpl() {
  set_client_for_logging(nullptr);
}

// mojom::RewardsEngine implementation begin (in the order of appearance in
// Mojom)
void RewardsEngineImpl::GetEnvironment(GetEnvironmentCallback callback) {
  std::move(callback).Run(_environment);
}

void RewardsEngineImpl::CreateRewardsWallet(
    const std::string& country,
    CreateRewardsWalletCallback callback) {
  wallet()->CreateWalletIfNecessary(
      country.empty() ? absl::nullopt
                      : absl::optional<std::string>(std::move(country)),
      std::move(callback));
}

void RewardsEngineImpl::GetRewardsParameters(
    GetRewardsParametersCallback callback) {
  auto params = state()->GetRewardsParameters();
  if (params->rate == 0.0) {
    // A rate of zero indicates that the rewards parameters have
    // not yet been successfully initialized from the server.
    BLOG(1, "Rewards parameters not set - fetching from server");
    api()->FetchParameters(std::move(callback));
    return;
  }

  std::move(callback).Run(std::move(params));
}

void RewardsEngineImpl::GetAutoContributeProperties(
    GetAutoContributePropertiesCallback callback) {
  auto props = mojom::AutoContributeProperties::New();
  props->enabled_contribute = state()->GetAutoContributeEnabled();
  props->amount = state()->GetAutoContributionAmount();
  props->contribution_min_time = state()->GetPublisherMinVisitTime();
  props->contribution_min_visits = state()->GetPublisherMinVisits();
  props->reconcile_stamp = state()->GetReconcileStamp();
  std::move(callback).Run(std::move(props));
}

void RewardsEngineImpl::GetPublisherMinVisitTime(
    GetPublisherMinVisitTimeCallback callback) {
  std::move(callback).Run(state()->GetPublisherMinVisitTime());
}

void RewardsEngineImpl::GetPublisherMinVisits(
    GetPublisherMinVisitsCallback callback) {
  std::move(callback).Run(state()->GetPublisherMinVisits());
}

void RewardsEngineImpl::GetAutoContributeEnabled(
    GetAutoContributeEnabledCallback callback) {
  std::move(callback).Run(state()->GetAutoContributeEnabled());
}

void RewardsEngineImpl::GetReconcileStamp(GetReconcileStampCallback callback) {
  std::move(callback).Run(state()->GetReconcileStamp());
}

void RewardsEngineImpl::OnLoad(mojom::VisitDataPtr visit_data,
                               uint64_t current_time) {
  if (!visit_data || visit_data->domain.empty()) {
    return;
  }

  auto iter = current_pages_.find(visit_data->tab_id);
  if (iter != current_pages_.end() &&
      iter->second.domain == visit_data->domain) {
    return;
  }

  if (last_shown_tab_id_ == visit_data->tab_id) {
    last_tab_active_time_ = current_time;
  }

  current_pages_[visit_data->tab_id] = *visit_data;
}

void RewardsEngineImpl::OnUnload(uint32_t tab_id, uint64_t current_time) {
  OnHide(tab_id, current_time);
  auto iter = current_pages_.find(tab_id);
  if (iter != current_pages_.end()) {
    current_pages_.erase(iter);
  }
}

void RewardsEngineImpl::OnShow(uint32_t tab_id, uint64_t current_time) {
  last_tab_active_time_ = current_time;
  last_shown_tab_id_ = tab_id;
}

void RewardsEngineImpl::OnHide(uint32_t tab_id, uint64_t current_time) {
  if (tab_id != last_shown_tab_id_ || last_tab_active_time_ == 0) {
    return;
  }

  auto iter = current_pages_.find(tab_id);
  if (iter == current_pages_.end()) {
    return;
  }

  const std::string type = media()->GetLinkType(iter->second.domain, "", "");
  uint64_t duration = current_time - last_tab_active_time_;
  last_tab_active_time_ = 0;

  if (type == GITHUB_MEDIA_TYPE) {
    base::flat_map<std::string, std::string> parts;
    parts["duration"] = std::to_string(duration);
    media()->ProcessMedia(parts, type, iter->second.Clone());
    return;
  }

  publisher()->SaveVisit(iter->second.domain, iter->second, duration, true, 0,
                         [](mojom::Result, mojom::PublisherInfoPtr) {});
}

void RewardsEngineImpl::OnForeground(uint32_t tab_id, uint64_t current_time) {
  // When performing automated testing, ignore changes in browser window
  // activation. When running tests in parallel, activation changes can
  // interfere with AC calculations on some platforms.
  if (is_testing) {
    return;
  }

  if (last_shown_tab_id_ != tab_id) {
    return;
  }

  OnShow(tab_id, current_time);
}

void RewardsEngineImpl::OnBackground(uint32_t tab_id, uint64_t current_time) {
  // When performing automated testing, ignore changes in browser window
  // activation. When running tests in parallel, activation changes can
  // interfere with AC calculations on some platforms.
  if (is_testing) {
    return;
  }

  OnHide(tab_id, current_time);
}

void RewardsEngineImpl::OnXHRLoad(
    uint32_t tab_id,
    const std::string& url,
    const base::flat_map<std::string, std::string>& parts,
    const std::string& first_party_url,
    const std::string& referrer,
    mojom::VisitDataPtr visit_data) {
  std::string type = media()->GetLinkType(url, first_party_url, referrer);
  if (type.empty()) {
    return;
  }
  media()->ProcessMedia(parts, type, std::move(visit_data));
}

void RewardsEngineImpl::SetPublisherExclude(
    const std::string& publisher_key,
    mojom::PublisherExclude exclude,
    SetPublisherExcludeCallback callback) {
  publisher()->SetPublisherExclude(publisher_key, exclude, std::move(callback));
}

void RewardsEngineImpl::RestorePublishers(RestorePublishersCallback callback) {
  database()->RestorePublishers(std::move(callback));
}

void RewardsEngineImpl::FetchPromotions(FetchPromotionsCallback callback) {
  promotion()->Fetch(std::move(callback));
}

void RewardsEngineImpl::ClaimPromotion(const std::string& promotion_id,
                                       const std::string& payload,
                                       ClaimPromotionCallback callback) {
  promotion()->Claim(promotion_id, payload, std::move(callback));
}

void RewardsEngineImpl::AttestPromotion(const std::string& promotion_id,
                                        const std::string& solution,
                                        AttestPromotionCallback callback) {
  promotion()->Attest(promotion_id, solution, std::move(callback));
}

void RewardsEngineImpl::SetPublisherMinVisitTime(int duration_in_seconds) {
  state()->SetPublisherMinVisitTime(duration_in_seconds);
}

void RewardsEngineImpl::SetPublisherMinVisits(int visits) {
  state()->SetPublisherMinVisits(visits);
}

void RewardsEngineImpl::SetAutoContributionAmount(double amount) {
  state()->SetAutoContributionAmount(amount);
}

void RewardsEngineImpl::SetAutoContributeEnabled(bool enabled) {
  state()->SetAutoContributeEnabled(enabled);
}

void RewardsEngineImpl::GetBalanceReport(mojom::ActivityMonth month,
                                         int32_t year,
                                         GetBalanceReportCallback callback) {
  database()->GetBalanceReportInfo(month, year,
                                   ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetPublisherActivityFromUrl(
    uint64_t window_id,
    mojom::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  publisher()->GetPublisherActivityFromUrl(window_id, std::move(visit_data),
                                           publisher_blob);
}

void RewardsEngineImpl::GetAutoContributionAmount(
    GetAutoContributionAmountCallback callback) {
  std::move(callback).Run(state()->GetAutoContributionAmount());
}

void RewardsEngineImpl::GetPublisherBanner(
    const std::string& publisher_id,
    GetPublisherBannerCallback callback) {
  publisher()->GetPublisherBanner(publisher_id,
                                  ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::OneTimeTip(const std::string& publisher_key,
                                   double amount,
                                   OneTimeTipCallback callback) {
  contribution()->OneTimeTip(publisher_key, amount,
                             ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::RemoveRecurringTip(
    const std::string& publisher_key,
    RemoveRecurringTipCallback callback) {
  database()->RemoveRecurringTip(publisher_key,
                                 ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetCreationStamp(GetCreationStampCallback callback) {
  std::move(callback).Run(state()->GetCreationStamp());
}

void RewardsEngineImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  auto info = mojom::RewardsInternalsInfo::New();

  mojom::RewardsWalletPtr wallet = wallet_.GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    std::move(callback).Run(std::move(info));
    return;
  }

  // Retrieve the payment id.
  info->payment_id = wallet->payment_id;

  // Retrieve the boot stamp.
  info->boot_stamp = state()->GetCreationStamp();

  // Retrieve the key info seed and validate it.
  if (!util::Security::IsSeedValid(wallet->recovery_seed)) {
    info->is_key_info_seed_valid = false;
  } else {
    std::vector<uint8_t> secret_key =
        util::Security::GetHKDF(wallet->recovery_seed);
    std::vector<uint8_t> public_key;
    std::vector<uint8_t> new_secret_key;
    info->is_key_info_seed_valid = util::Security::GetPublicKeyFromSeed(
        secret_key, &public_key, &new_secret_key);
  }

  std::move(callback).Run(std::move(info));
}

void RewardsEngineImpl::SaveRecurringTip(mojom::RecurringTipPtr info,
                                         SaveRecurringTipCallback callback) {
  database()->SaveRecurringTip(
      std::move(info), [this, callback = ToLegacyCallback(std::move(callback))](
                           mojom::Result result) {
        contribution()->SetMonthlyContributionTimer();
        callback(result);
      });
}

void RewardsEngineImpl::SendContribution(const std::string& publisher_id,
                                         double amount,
                                         bool set_monthly,
                                         SendContributionCallback callback) {
  contribution()->SendContribution(publisher_id, amount, set_monthly,
                                   std::move(callback));
}

void RewardsEngineImpl::GetRecurringTips(GetRecurringTipsCallback callback) {
  contribution()->GetRecurringTips(ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetOneTimeTips(GetOneTimeTipsCallback callback) {
  database()->GetOneTimeTips(util::GetCurrentMonth(), util::GetCurrentYear(),
                             ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    mojom::ActivityInfoFilterPtr filter,
    GetActivityInfoListCallback callback) {
  database()->GetActivityInfoList(start, limit, std::move(filter),
                                  ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetPublishersVisitedCount(
    GetPublishersVisitedCountCallback callback) {
  database()->GetPublishersVisitedCount(std::move(callback));
}

void RewardsEngineImpl::GetExcludedList(GetExcludedListCallback callback) {
  database()->GetExcludedList(ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::RefreshPublisher(const std::string& publisher_key,
                                         RefreshPublisherCallback callback) {
  publisher()->RefreshPublisher(publisher_key,
                                ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::StartContributionsForTesting() {
  contribution()->StartContributionsForTesting();  // IN-TEST
}

void RewardsEngineImpl::UpdateMediaDuration(uint64_t window_id,
                                            const std::string& publisher_key,
                                            uint64_t duration,
                                            bool first_visit) {
  publisher()->UpdateMediaDuration(window_id, publisher_key, duration,
                                   first_visit);
}

void RewardsEngineImpl::IsPublisherRegistered(
    const std::string& publisher_id,
    IsPublisherRegisteredCallback callback) {
  publisher()->GetServerPublisherInfo(
      publisher_id, true /* use_prefix_list */,
      [callback = ToLegacyCallback(std::move(callback))](
          mojom::ServerPublisherInfoPtr info) {
        callback(info && info->status != mojom::PublisherStatus::NOT_VERIFIED);
      });
}

void RewardsEngineImpl::GetPublisherInfo(const std::string& publisher_key,
                                         GetPublisherInfoCallback callback) {
  database()->GetPublisherInfo(publisher_key,
                               ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetPublisherPanelInfo(
    const std::string& publisher_key,
    GetPublisherPanelInfoCallback callback) {
  publisher()->GetPublisherPanelInfo(publisher_key,
                                     ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::SavePublisherInfo(
    uint64_t window_id,
    mojom::PublisherInfoPtr publisher_info,
    SavePublisherInfoCallback callback) {
  publisher()->SavePublisherInfo(window_id, std::move(publisher_info),
                                 ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::SetInlineTippingPlatformEnabled(
    mojom::InlineTipsPlatforms platform,
    bool enabled) {
  state()->SetInlineTippingPlatformEnabled(platform, enabled);
}

void RewardsEngineImpl::GetInlineTippingPlatformEnabled(
    mojom::InlineTipsPlatforms platform,
    GetInlineTippingPlatformEnabledCallback callback) {
  std::move(callback).Run(state()->GetInlineTippingPlatformEnabled(platform));
}

void RewardsEngineImpl::GetShareURL(
    const base::flat_map<std::string, std::string>& args,
    GetShareURLCallback callback) {
  std::move(callback).Run(publisher()->GetShareURL(args));
}

void RewardsEngineImpl::FetchBalance(FetchBalanceCallback callback) {
  wallet()->FetchBalance(std::move(callback));
}

void RewardsEngineImpl::GetExternalWallet(const std::string& wallet_type,
                                          GetExternalWalletCallback callback) {
  if (wallet_type == constant::kWalletBitflyer) {
    return bitflyer()->GetWallet(std::move(callback));
  }

  if (wallet_type == constant::kWalletGemini) {
    return gemini()->GetWallet(std::move(callback));
  }

  if (wallet_type == constant::kWalletUphold) {
    return uphold()->GetWallet(std::move(callback));
  }

  if (wallet_type == constant::kWalletZebPay) {
    return zebpay()->GetWallet(std::move(callback));
  }

  NOTREACHED() << "Unknown external wallet type!";
  std::move(callback).Run(
      base::unexpected(mojom::GetExternalWalletError::kUnexpected));
}

void RewardsEngineImpl::ConnectExternalWallet(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ConnectExternalWalletCallback callback) {
  if (wallet_type == constant::kWalletBitflyer) {
    return bitflyer()->ConnectWallet(args, std::move(callback));
  }

  if (wallet_type == constant::kWalletGemini) {
    return gemini()->ConnectWallet(args, std::move(callback));
  }

  if (wallet_type == constant::kWalletUphold) {
    return uphold()->ConnectWallet(args, std::move(callback));
  }

  if (wallet_type == constant::kWalletZebPay) {
    return zebpay()->ConnectWallet(args, std::move(callback));
  }

  NOTREACHED() << "Unknown external wallet type!";
  std::move(callback).Run(
      base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
}

void RewardsEngineImpl::GetTransactionReport(
    mojom::ActivityMonth month,
    int year,
    GetTransactionReportCallback callback) {
  database()->GetTransactionReport(month, year,
                                   ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetContributionReport(
    mojom::ActivityMonth month,
    int year,
    GetContributionReportCallback callback) {
  database()->GetContributionReport(month, year,
                                    ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetAllContributions(
    GetAllContributionsCallback callback) {
  database()->GetAllContributions(ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetMonthlyReport(mojom::ActivityMonth month,
                                         int year,
                                         GetMonthlyReportCallback callback) {
  report()->GetMonthly(month, year, ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetAllMonthlyReportIds(
    GetAllMonthlyReportIdsCallback callback) {
  report()->GetAllMonthlyIds(ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetAllPromotions(GetAllPromotionsCallback callback) {
  database()->GetAllPromotions(ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::Shutdown(ShutdownCallback callback) {
  client_->ClearAllNotifications();

  database()->FinishAllInProgressContributions(
      std::bind(&RewardsEngineImpl::OnAllDone, this, _1,
                ToLegacyCallback(std::move(callback))));
}

void RewardsEngineImpl::GetEventLogs(GetEventLogsCallback callback) {
  database()->GetLastEventLogs(ToLegacyCallback(std::move(callback)));
}

void RewardsEngineImpl::GetRewardsWallet(GetRewardsWalletCallback callback) {
  auto rewards_wallet = wallet()->GetWallet();
  if (rewards_wallet) {
    // While the wallet creation flow is running, the Rewards wallet data may
    // have a recovery seed without a payment ID. Only return a struct to the
    // caller if it contains a payment ID.
    if (rewards_wallet->payment_id.empty()) {
      rewards_wallet = nullptr;
    }
  }
  std::move(callback).Run(std::move(rewards_wallet));
}
// mojom::RewardsEngine implementation end

// mojom::RewardsEngineClient helpers begin (in the order of appearance in
// Mojom)
std::string RewardsEngineImpl::GetClientCountryCode() {
  std::string country_code;
  client_->GetClientCountryCode(&country_code);
  return country_code;
}

bool RewardsEngineImpl::IsAutoContributeSupportedForClient() {
  bool value = false;
  client_->IsAutoContributeSupportedForClient(&value);
  return value;
}

std::string RewardsEngineImpl::GetLegacyWallet() {
  std::string wallet;
  client_->GetLegacyWallet(&wallet);
  return wallet;
}

mojom::ClientInfoPtr RewardsEngineImpl::GetClientInfo() {
  auto info = mojom::ClientInfo::New();
  client_->GetClientInfo(&info);
  return info;
}

absl::optional<std::string> RewardsEngineImpl::EncryptString(
    const std::string& value) {
  absl::optional<std::string> result;
  client_->EncryptString(value, &result);
  return result;
}

absl::optional<std::string> RewardsEngineImpl::DecryptString(
    const std::string& value) {
  absl::optional<std::string> result;
  client_->DecryptString(value, &result);
  return result;
}
// mojom::RewardsEngineClient helpers end

mojom::RewardsEngineClient* RewardsEngineImpl::client() {
  return client_.get();
}

database::Database* RewardsEngineImpl::database() {
  return &database_;
}

void RewardsEngineImpl::InitializeDatabase(ResultCallback callback) {
  auto finish_callback =
      base::BindOnce(&RewardsEngineImpl::OnInitialized, base::Unretained(this),
                     std::move(callback));

  auto database_callback =
      base::BindOnce(&RewardsEngineImpl::OnDatabaseInitialized,
                     base::Unretained(this), std::move(finish_callback));

  database()->Initialize(ToLegacyCallback(std::move(database_callback)));
}

void RewardsEngineImpl::OnDatabaseInitialized(ResultCallback callback,
                                              mojom::Result result) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Database could not be initialized. Error: " << result);
    return std::move(callback).Run(result);
  }

  state()->Initialize(base::BindOnce(&RewardsEngineImpl::OnStateInitialized,
                                     base::Unretained(this),
                                     std::move(callback)));
}

void RewardsEngineImpl::OnStateInitialized(ResultCallback callback,
                                           mojom::Result result) {
  if (result != mojom::Result::OK) {
    BLOG(0, "States could not be initialized. Error: " << result);
  }

  std::move(callback).Run(result);
}

void RewardsEngineImpl::OnInitialized(ResultCallback callback,
                                      mojom::Result result) {
  if (result == mojom::Result::OK) {
    StartServices();
  } else {
    BLOG(0, "Failed to initialize RewardsEngineImpl: " << result);
  }

  std::move(callback).Run(result);
}

void RewardsEngineImpl::StartServices() {
  publisher()->SetPublisherServerListTimer();
  contribution()->SetAutoContributeTimer();
  contribution()->SetMonthlyContributionTimer();
  promotion()->Refresh(false);
  contribution()->Initialize();
  promotion()->Initialize();
  api()->Initialize();
  recovery_.Check();
}

void RewardsEngineImpl::OnAllDone(mojom::Result result,
                                  LegacyResultCallback callback) {
  database()->Close(std::move(callback));
}

}  // namespace brave_rewards::internal
