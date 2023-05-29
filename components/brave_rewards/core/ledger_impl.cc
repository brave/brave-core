/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/ledger_impl.h"

#include <memory>
#include <vector>

#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/components/brave_rewards/core/common/legacy_callback_helpers.h"
#include "brave/components/brave_rewards/core/common/security_util.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/legacy/static_values.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "brave/components/brave_rewards/core/publisher/publisher_status_helper.h"

using std::placeholders::_1;

namespace brave_rewards::internal {

LedgerImpl::~LedgerImpl() {
  VLOG(0) << "~LedgerImpl()";
}

// mojom::Ledger implementation begin (in the order of appearance in Mojom)
void LedgerImpl::Initialize(InitializeCallback callback) {
  if (ready_state_ != ReadyState::kUninitialized) {
    BLOG(0, "Ledger already initializing");
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  ready_state_ = ReadyState::kInitializing;
  InitializeDatabase(ToLegacyCallback(std::move(callback)));
}

void LedgerImpl::SetEnvironment(mojom::Environment environment) {
  DCHECK(IsUninitialized() || is_testing_);
  environment_ = environment;
}

void LedgerImpl::SetDebug(bool is_debug) {
  DCHECK(IsUninitialized() || is_testing_);
  is_debug_ = is_debug;
}

void LedgerImpl::SetReconcileInterval(int32_t interval) {
  DCHECK(IsUninitialized() || is_testing_);
  reconcile_interval_ = interval;
}

void LedgerImpl::SetRetryInterval(int32_t interval) {
  DCHECK(IsUninitialized() || is_testing_);
  retry_interval_ = interval;
}

void LedgerImpl::SetTesting(bool is_testing) {
  is_testing_ = is_testing;
}

void LedgerImpl::SetStateMigrationTargetVersionForTesting(int32_t version) {
  DCHECK(is_testing_);
  state_migration_target_version_for_testing_ = version;
}

void LedgerImpl::GetEnvironment(GetEnvironmentCallback callback) {
  std::move(callback).Run(GetEnvironment());
}

void LedgerImpl::GetDebug(GetDebugCallback callback) {
  std::move(callback).Run(GetDebug());
}

void LedgerImpl::GetReconcileInterval(GetReconcileIntervalCallback callback) {
  std::move(callback).Run(GetReconcileInterval());
}

void LedgerImpl::GetRetryInterval(GetRetryIntervalCallback callback) {
  std::move(callback).Run(GetRetryInterval());
}

void LedgerImpl::CreateRewardsWallet(const std::string& country,
                                     CreateRewardsWalletCallback callback) {
  WhenReady([this, country, callback = std::move(callback)]() mutable {
    wallet()->CreateWalletIfNecessary(
        country.empty() ? absl::nullopt
                        : absl::optional<std::string>(std::move(country)),
        std::move(callback));
  });
}

void LedgerImpl::GetRewardsParameters(GetRewardsParametersCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    auto params = state()->GetRewardsParameters();
    if (params->rate == 0.0) {
      // A rate of zero indicates that the rewards parameters have
      // not yet been successfully initialized from the server.
      BLOG(1, "Rewards parameters not set - fetching from server");
      api()->FetchParameters(std::move(callback));
      return;
    }

    std::move(callback).Run(std::move(params));
  });
}

void LedgerImpl::GetAutoContributeProperties(
    GetAutoContributePropertiesCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(mojom::AutoContributeProperties::New());
  }

  auto props = mojom::AutoContributeProperties::New();
  props->enabled_contribute = state()->GetAutoContributeEnabled();
  props->amount = state()->GetAutoContributionAmount();
  props->contribution_min_time = state()->GetPublisherMinVisitTime();
  props->contribution_min_visits = state()->GetPublisherMinVisits();
  props->reconcile_stamp = state()->GetReconcileStamp();
  std::move(callback).Run(std::move(props));
}

void LedgerImpl::GetPublisherMinVisitTime(
    GetPublisherMinVisitTimeCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetPublisherMinVisitTime());
}

void LedgerImpl::GetPublisherMinVisits(GetPublisherMinVisitsCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetPublisherMinVisits());
}

void LedgerImpl::GetAutoContributeEnabled(
    GetAutoContributeEnabledCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(false);
  }

  std::move(callback).Run(state()->GetAutoContributeEnabled());
}

void LedgerImpl::GetReconcileStamp(GetReconcileStampCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetReconcileStamp());
}

void LedgerImpl::OnLoad(mojom::VisitDataPtr visit_data, uint64_t current_time) {
  if (!IsReady() || !visit_data || visit_data->domain.empty()) {
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

void LedgerImpl::OnUnload(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  OnHide(tab_id, current_time);
  auto iter = current_pages_.find(tab_id);
  if (iter != current_pages_.end()) {
    current_pages_.erase(iter);
  }
}

void LedgerImpl::OnShow(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  last_tab_active_time_ = current_time;
  last_shown_tab_id_ = tab_id;
}

void LedgerImpl::OnHide(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

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

void LedgerImpl::OnForeground(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  // When performing automated testing, ignore changes in browser window
  // activation. When running tests in parallel, activation changes can
  // interfere with AC calculations on some platforms.
  if (is_testing_) {
    return;
  }

  if (last_shown_tab_id_ != tab_id) {
    return;
  }

  OnShow(tab_id, current_time);
}

void LedgerImpl::OnBackground(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  // When performing automated testing, ignore changes in browser window
  // activation. When running tests in parallel, activation changes can
  // interfere with AC calculations on some platforms.
  if (is_testing_) {
    return;
  }

  OnHide(tab_id, current_time);
}

void LedgerImpl::OnXHRLoad(
    uint32_t tab_id,
    const std::string& url,
    const base::flat_map<std::string, std::string>& parts,
    const std::string& first_party_url,
    const std::string& referrer,
    mojom::VisitDataPtr visit_data) {
  if (!IsReady()) {
    return;
  }

  std::string type = media()->GetLinkType(url, first_party_url, referrer);
  if (type.empty()) {
    return;
  }
  media()->ProcessMedia(parts, type, std::move(visit_data));
}

void LedgerImpl::SetPublisherExclude(const std::string& publisher_key,
                                     mojom::PublisherExclude exclude,
                                     SetPublisherExcludeCallback callback) {
  WhenReady(
      [this, publisher_key, exclude, callback = std::move(callback)]() mutable {
        publisher()->SetPublisherExclude(publisher_key, exclude,
                                         std::move(callback));
      });
}

void LedgerImpl::RestorePublishers(RestorePublishersCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->RestorePublishers(std::move(callback));
  });
}

void LedgerImpl::FetchPromotions(FetchPromotionsCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    promotion()->Fetch(std::move(callback));
  });
}

void LedgerImpl::ClaimPromotion(const std::string& promotion_id,
                                const std::string& payload,
                                ClaimPromotionCallback callback) {
  WhenReady(
      [this, promotion_id, payload, callback = std::move(callback)]() mutable {
        promotion()->Claim(promotion_id, payload, std::move(callback));
      });
}

void LedgerImpl::AttestPromotion(const std::string& promotion_id,
                                 const std::string& solution,
                                 AttestPromotionCallback callback) {
  WhenReady(
      [this, promotion_id, solution, callback = std::move(callback)]() mutable {
        promotion()->Attest(promotion_id, solution, std::move(callback));
      });
}

void LedgerImpl::SetPublisherMinVisitTime(int duration_in_seconds) {
  WhenReady([this, duration_in_seconds]() {
    state()->SetPublisherMinVisitTime(duration_in_seconds);
  });
}

void LedgerImpl::SetPublisherMinVisits(int visits) {
  WhenReady([this, visits]() { state()->SetPublisherMinVisits(visits); });
}

void LedgerImpl::SetAutoContributionAmount(double amount) {
  WhenReady([this, amount]() { state()->SetAutoContributionAmount(amount); });
}

void LedgerImpl::SetAutoContributeEnabled(bool enabled) {
  WhenReady([this, enabled]() { state()->SetAutoContributeEnabled(enabled); });
}

void LedgerImpl::GetBalanceReport(mojom::ActivityMonth month,
                                  int32_t year,
                                  GetBalanceReportCallback callback) {
  WhenReady([this, month, year,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetBalanceReportInfo(month, year, std::move(callback));
  });
}

void LedgerImpl::GetPublisherActivityFromUrl(
    uint64_t window_id,
    mojom::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  WhenReady([this, window_id, visit_data = std::move(visit_data),
             publisher_blob]() mutable {
    publisher()->GetPublisherActivityFromUrl(window_id, std::move(visit_data),
                                             publisher_blob);
  });
}

void LedgerImpl::GetAutoContributionAmount(
    GetAutoContributionAmountCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetAutoContributionAmount());
}

void LedgerImpl::GetPublisherBanner(const std::string& publisher_id,
                                    GetPublisherBannerCallback callback) {
  WhenReady([this, publisher_id,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    publisher()->GetPublisherBanner(publisher_id, std::move(callback));
  });
}

void LedgerImpl::OneTimeTip(const std::string& publisher_key,
                            double amount,
                            OneTimeTipCallback callback) {
  WhenReady([this, publisher_key, amount,
             callback = ToLegacyCallback(std::move(callback))]() {
    contribution()->OneTimeTip(publisher_key, amount, std::move(callback));
  });
}

void LedgerImpl::RemoveRecurringTip(const std::string& publisher_key,
                                    RemoveRecurringTipCallback callback) {
  WhenReady([this, publisher_key,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->RemoveRecurringTip(publisher_key, std::move(callback));
  });
}

void LedgerImpl::GetCreationStamp(GetCreationStampCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetCreationStamp());
}

void LedgerImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
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
  });
}

void LedgerImpl::SaveRecurringTip(mojom::RecurringTipPtr info,
                                  SaveRecurringTipCallback callback) {
  WhenReady([this, info = std::move(info),
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->SaveRecurringTip(
        std::move(info),
        [this, callback = std::move(callback)](mojom::Result result) {
          contribution()->SetMonthlyContributionTimer();
          callback(result);
        });
  });
}

void LedgerImpl::SendContribution(const std::string& publisher_id,
                                  double amount,
                                  bool set_monthly,
                                  SendContributionCallback callback) {
  WhenReady([this, publisher_id, amount, set_monthly,
             callback = std::move(callback)]() mutable {
    contribution()->SendContribution(publisher_id, amount, set_monthly,
                                     std::move(callback));
  });
}

void LedgerImpl::GetRecurringTips(GetRecurringTipsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    contribution()->GetRecurringTips(std::move(callback));
  });
}

void LedgerImpl::GetOneTimeTips(GetOneTimeTipsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetOneTimeTips(util::GetCurrentMonth(), util::GetCurrentYear(),
                               std::move(callback));
  });
}

void LedgerImpl::GetActivityInfoList(uint32_t start,
                                     uint32_t limit,
                                     mojom::ActivityInfoFilterPtr filter,
                                     GetActivityInfoListCallback callback) {
  WhenReady([this, start, limit, filter = std::move(filter),
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetActivityInfoList(start, limit, std::move(filter),
                                    std::move(callback));
  });
}

void LedgerImpl::GetPublishersVisitedCount(
    GetPublishersVisitedCountCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->GetPublishersVisitedCount(std::move(callback));
  });
}

void LedgerImpl::GetExcludedList(GetExcludedListCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetExcludedList(std::move(callback));
  });
}

void LedgerImpl::RefreshPublisher(const std::string& publisher_key,
                                  RefreshPublisherCallback callback) {
  WhenReady([this, publisher_key,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    publisher()->RefreshPublisher(publisher_key, std::move(callback));
  });
}

void LedgerImpl::StartContributionsForTesting() {
  WhenReady([this]() {
    contribution()->StartContributionsForTesting();  // IN-TEST
  });
}

void LedgerImpl::UpdateMediaDuration(uint64_t window_id,
                                     const std::string& publisher_key,
                                     uint64_t duration,
                                     bool first_visit) {
  WhenReady([this, window_id, publisher_key, duration, first_visit]() {
    publisher()->UpdateMediaDuration(window_id, publisher_key, duration,
                                     first_visit);
  });
}

void LedgerImpl::IsPublisherRegistered(const std::string& publisher_id,
                                       IsPublisherRegisteredCallback callback) {
  WhenReady([this, publisher_id,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    publisher()->GetServerPublisherInfo(
        publisher_id, true /* use_prefix_list */,
        [callback = std::move(callback)](mojom::ServerPublisherInfoPtr info) {
          callback(info &&
                   info->status != mojom::PublisherStatus::NOT_VERIFIED);
        });
  });
}

void LedgerImpl::GetPublisherInfo(const std::string& publisher_key,
                                  GetPublisherInfoCallback callback) {
  WhenReady([this, publisher_key,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetPublisherInfo(publisher_key, std::move(callback));
  });
}

void LedgerImpl::GetPublisherPanelInfo(const std::string& publisher_key,
                                       GetPublisherPanelInfoCallback callback) {
  WhenReady([this, publisher_key,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    publisher()->GetPublisherPanelInfo(publisher_key, std::move(callback));
  });
}

void LedgerImpl::SavePublisherInfo(uint64_t window_id,
                                   mojom::PublisherInfoPtr publisher_info,
                                   SavePublisherInfoCallback callback) {
  WhenReady([this, window_id, info = std::move(publisher_info),
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    publisher()->SavePublisherInfo(window_id, std::move(info),
                                   std::move(callback));
  });
}

void LedgerImpl::SetInlineTippingPlatformEnabled(
    mojom::InlineTipsPlatforms platform,
    bool enabled) {
  WhenReady([this, platform, enabled]() {
    state()->SetInlineTippingPlatformEnabled(platform, enabled);
  });
}

void LedgerImpl::GetInlineTippingPlatformEnabled(
    mojom::InlineTipsPlatforms platform,
    GetInlineTippingPlatformEnabledCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(false);
  }

  std::move(callback).Run(state()->GetInlineTippingPlatformEnabled(platform));
}

void LedgerImpl::GetShareURL(
    const base::flat_map<std::string, std::string>& args,
    GetShareURLCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run("");
  }

  std::move(callback).Run(publisher()->GetShareURL(args));
}

void LedgerImpl::GetPendingContributions(
    GetPendingContributionsCallback callback) {
  std::move(callback).Run({});
}

void LedgerImpl::RemovePendingContribution(
    uint64_t id,
    RemovePendingContributionCallback callback) {
  std::move(callback).Run(mojom::Result::LEDGER_OK);
}

void LedgerImpl::RemoveAllPendingContributions(
    RemovePendingContributionCallback callback) {
  std::move(callback).Run(mojom::Result::LEDGER_OK);
}

void LedgerImpl::GetPendingContributionsTotal(
    GetPendingContributionsTotalCallback callback) {
  std::move(callback).Run(0);
}

void LedgerImpl::FetchBalance(FetchBalanceCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    wallet()->FetchBalance(std::move(callback));
  });
}

void LedgerImpl::GetExternalWallet(const std::string& wallet_type,
                                   GetExternalWalletCallback callback) {
  WhenReady([this, wallet_type, callback = std::move(callback)]() mutable {
    if (wallet_type == constant::kWalletBitflyer) {
      return bitflyer()->GetWallet(std::move(callback));
    }

    if (wallet_type == constant::kWalletGemini) {
      return gemini()->GetWallet(std::move(callback));
    }

    if (wallet_type == constant::kWalletUphold) {
      return uphold()->GetWallet(std::move(callback));
    }

    NOTREACHED() << "Unknown external wallet type!";
    std::move(callback).Run(
        base::unexpected(mojom::GetExternalWalletError::kUnexpected));
  });
}

void LedgerImpl::ConnectExternalWallet(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ConnectExternalWalletCallback callback) {
  WhenReady(
      [this, wallet_type, args, callback = std::move(callback)]() mutable {
        if (wallet_type == constant::kWalletBitflyer) {
          return bitflyer()->ConnectWallet(args, std::move(callback));
        }

        if (wallet_type == constant::kWalletGemini) {
          return gemini()->ConnectWallet(args, std::move(callback));
        }

        if (wallet_type == constant::kWalletUphold) {
          return uphold()->ConnectWallet(args, std::move(callback));
        }

        NOTREACHED() << "Unknown external wallet type!";
        std::move(callback).Run(
            base::unexpected(mojom::ConnectExternalWalletError::kUnexpected));
      });
}

void LedgerImpl::GetTransactionReport(mojom::ActivityMonth month,
                                      int year,
                                      GetTransactionReportCallback callback) {
  WhenReady([this, month, year,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetTransactionReport(month, year, std::move(callback));
  });
}

void LedgerImpl::GetContributionReport(mojom::ActivityMonth month,
                                       int year,
                                       GetContributionReportCallback callback) {
  WhenReady([this, month, year,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetContributionReport(month, year, std::move(callback));
  });
}

void LedgerImpl::GetAllContributions(GetAllContributionsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetAllContributions(std::move(callback));
  });
}

void LedgerImpl::GetMonthlyReport(mojom::ActivityMonth month,
                                  int year,
                                  GetMonthlyReportCallback callback) {
  WhenReady([this, month, year,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    report()->GetMonthly(month, year, std::move(callback));
  });
}

void LedgerImpl::GetAllMonthlyReportIds(
    GetAllMonthlyReportIdsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    report()->GetAllMonthlyIds(std::move(callback));
  });
}

void LedgerImpl::GetAllPromotions(GetAllPromotionsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetAllPromotions(std::move(callback));
  });
}

void LedgerImpl::Shutdown(ShutdownCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  ready_state_ = ReadyState::kShuttingDown;
  ledger_client_->ClearAllNotifications();

  database()->FinishAllInProgressContributions(std::bind(
      &LedgerImpl::OnAllDone, this, _1, ToLegacyCallback(std::move(callback))));
}

void LedgerImpl::GetEventLogs(GetEventLogsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetLastEventLogs(std::move(callback));
  });
}

void LedgerImpl::GetRewardsWallet(GetRewardsWalletCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
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
  });
}
// mojom::Ledger implementation end

// mojom::LedgerClient helpers begin (in the order of appearance in Mojom)
bool LedgerImpl::IsBitFlyerRegion() {
  bool value = false;
  ledger_client_->IsBitFlyerRegion(&value);
  return value;
}

std::string LedgerImpl::GetLegacyWallet() {
  std::string wallet;
  ledger_client_->GetLegacyWallet(&wallet);
  return wallet;
}

mojom::ClientInfoPtr LedgerImpl::GetClientInfo() {
  auto info = mojom::ClientInfo::New();
  ledger_client_->GetClientInfo(&info);
  return info;
}

absl::optional<std::string> LedgerImpl::EncryptString(
    const std::string& value) {
  absl::optional<std::string> result;
  ledger_client_->EncryptString(value, &result);
  return result;
}

absl::optional<std::string> LedgerImpl::DecryptString(
    const std::string& value) {
  absl::optional<std::string> result;
  ledger_client_->DecryptString(value, &result);
  return result;
}
// mojom::LedgerClient helpers end

mojom::LedgerClient* LedgerImpl::client() {
  return ledger_client_.get();
}

database::Database* LedgerImpl::database() {
  return &database_;
}

void LedgerImpl::SetDatabaseMigrationTargetVersionForTesting(uint32_t version) {
  DCHECK(is_testing_);
  database_migration_target_version_for_testing_ = version;
}

mojom::Environment LedgerImpl::GetEnvironment() {
  return environment_;
}

bool LedgerImpl::GetDebug() {
  return is_debug_;
}

int32_t LedgerImpl::GetReconcileInterval() {
  return reconcile_interval_;
}

int32_t LedgerImpl::GetRetryInterval() {
  return retry_interval_;
}

bool LedgerImpl::GetTesting() {
  return is_testing_;
}

int32_t LedgerImpl::GetStateMigrationTargetVersionForTesting() {
  return state_migration_target_version_for_testing_;
}

uint32_t LedgerImpl::GetDatabaseMigrationTargetVersionForTesting() {
  return database_migration_target_version_for_testing_;
}

bool LedgerImpl::IsShuttingDown() const {
  return ready_state_ == ReadyState::kShuttingDown;
}

bool LedgerImpl::IsUninitialized() const {
  return ready_state_ == ReadyState::kUninitialized;
}

LedgerImpl::LedgerImpl(
    mojo::PendingAssociatedRemote<mojom::LedgerClient> remote)
    : ledger_client_(std::move(remote)) {
  VLOG(0) << "LedgerImpl()";
}

bool LedgerImpl::IsReady() const {
  return ready_state_ == ReadyState::kReady;
}

void LedgerImpl::InitializeDatabase(LegacyResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  LegacyResultCallback finish_callback =
      std::bind(&LedgerImpl::OnInitialized, this, _1, std::move(callback));

  auto database_callback =
      std::bind(&LedgerImpl::OnDatabaseInitialized, this, _1, finish_callback);
  database()->Initialize(database_callback);
}

void LedgerImpl::OnDatabaseInitialized(mojom::Result result,
                                       LegacyResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Database could not be initialized. Error: " << result);
    callback(result);
    return;
  }

  state()->Initialize(base::BindOnce(&LedgerImpl::OnStateInitialized,
                                     base::Unretained(this),
                                     std::move(callback)));
}

void LedgerImpl::OnStateInitialized(LegacyResultCallback callback,
                                    mojom::Result result) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Failed to initialize state");
    callback(result);
    return;
  }

  callback(mojom::Result::LEDGER_OK);
}

void LedgerImpl::OnInitialized(mojom::Result result,
                               LegacyResultCallback callback) {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  if (result == mojom::Result::LEDGER_OK) {
    StartServices();
  } else {
    BLOG(0, "Failed to initialize wallet " << result);
  }

  while (!ready_callbacks_.empty()) {
    auto ready_callback = std::move(ready_callbacks_.front());
    ready_callbacks_.pop();
    ready_callback();
  }

  ready_state_ = ReadyState::kReady;

  callback(result);
}

void LedgerImpl::StartServices() {
  DCHECK(ready_state_ == ReadyState::kInitializing);

  publisher()->SetPublisherServerListTimer();
  contribution()->SetAutoContributeTimer();
  contribution()->SetMonthlyContributionTimer();
  promotion()->Refresh(false);
  contribution()->Initialize();
  promotion()->Initialize();
  api()->Initialize();
  recovery_.Check();
}

void LedgerImpl::OnAllDone(mojom::Result result,
                           LegacyResultCallback callback) {
  database()->Close(std::move(callback));
}

template <typename T>
void LedgerImpl::WhenReady(T callback) {
  switch (ready_state_) {
    case ReadyState::kReady:
      callback();
      break;
    case ReadyState::kShuttingDown:
      NOTREACHED();
      break;
    default:
      ready_callbacks_.push(std::function<void()>(
          [shared_callback = std::make_shared<T>(std::move(callback))]() {
            (*shared_callback)();
          }));
      break;
  }
}

}  // namespace brave_rewards::internal
