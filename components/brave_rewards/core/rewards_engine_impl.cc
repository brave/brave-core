/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

#include <vector>

#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/components/brave_rewards/core/api/api.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/common/callback_helpers.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/signer.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/gemini/gemini.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/initialization_manager.h"
#include "brave/components/brave_rewards/core/legacy/media/media.h"
#include "brave/components/brave_rewards/core/legacy/static_values.h"
#include "brave/components/brave_rewards/core/promotion/promotion.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/recovery/recovery.h"
#include "brave/components/brave_rewards/core/report/report.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/linkage_checker.h"
#include "brave/components/brave_rewards/core/wallet_provider/solana/solana_wallet_provider.h"
#include "brave/components/brave_rewards/core/wallet_provider/wallet_provider.h"
#include "brave/components/brave_rewards/core/zebpay/zebpay.h"

namespace brave_rewards::internal {

RewardsEngineImpl::RewardsEngineImpl(
    mojo::PendingAssociatedRemote<mojom::RewardsEngineClient> client_remote,
    const mojom::RewardsEngineOptions& options)
    : client_(std::move(client_remote)),
      options_(options),
      helpers_(std::make_unique<EnvironmentConfig>(*this),
               std::make_unique<InitializationManager>(*this),
               std::make_unique<URLLoader>(*this),
               std::make_unique<LinkageChecker>(*this),
               std::make_unique<SolanaWalletProvider>(*this)),
      promotion_(std::make_unique<promotion::Promotion>(*this)),
      publisher_(std::make_unique<publisher::Publisher>(*this)),
      media_(std::make_unique<Media>(*this)),
      contribution_(std::make_unique<contribution::Contribution>(*this)),
      wallet_(std::make_unique<wallet::Wallet>(*this)),
      database_(std::make_unique<database::Database>(*this)),
      report_(std::make_unique<report::Report>(*this)),
      state_(std::make_unique<state::State>(*this)),
      api_(std::make_unique<api::API>(*this)),
      recovery_(std::make_unique<recovery::Recovery>(*this)),
      bitflyer_(std::make_unique<bitflyer::Bitflyer>(*this)),
      gemini_(std::make_unique<gemini::Gemini>(*this)),
      uphold_(std::make_unique<uphold::Uphold>(*this)),
      zebpay_(std::make_unique<zebpay::ZebPay>(*this)) {
  DCHECK(base::ThreadPoolInstance::Get());
  set_client_for_logging(client_.get());
}

RewardsEngineImpl::~RewardsEngineImpl() {
  set_client_for_logging(nullptr);
}

// mojom::RewardsEngine implementation begin (in the order of appearance in
// Mojom)
void RewardsEngineImpl::Initialize(InitializeCallback callback) {
  Get<InitializationManager>().Initialize(
      base::BindOnce(&RewardsEngineImpl::OnInitializationComplete,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void RewardsEngineImpl::GetEnvironment(GetEnvironmentCallback callback) {
  std::move(callback).Run(Get<EnvironmentConfig>().current_environment());
}

void RewardsEngineImpl::CreateRewardsWallet(
    const std::string& country,
    CreateRewardsWalletCallback callback) {
  WhenReady([this, country, callback = std::move(callback)]() mutable {
    wallet()->CreateWalletIfNecessary(
        country.empty() ? std::nullopt
                        : std::optional<std::string>(std::move(country)),
        std::move(callback));
  });
}

void RewardsEngineImpl::GetRewardsParameters(
    GetRewardsParametersCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    auto params = state()->GetRewardsParameters();
    if (params->rate == 0.0) {
      // A rate of zero indicates that the rewards parameters have
      // not yet been successfully initialized from the server.
      Log(FROM_HERE) << "Rewards parameters not set - fetching from server";
      api()->FetchParameters(std::move(callback));
      return;
    }

    std::move(callback).Run(std::move(params));
  });
}

void RewardsEngineImpl::GetAutoContributeProperties(
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

void RewardsEngineImpl::GetPublisherMinVisitTime(
    GetPublisherMinVisitTimeCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetPublisherMinVisitTime());
}

void RewardsEngineImpl::GetPublisherMinVisits(
    GetPublisherMinVisitsCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetPublisherMinVisits());
}

void RewardsEngineImpl::GetAutoContributeEnabled(
    GetAutoContributeEnabledCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(false);
  }

  std::move(callback).Run(state()->GetAutoContributeEnabled());
}

void RewardsEngineImpl::GetReconcileStamp(GetReconcileStampCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetReconcileStamp());
}

void RewardsEngineImpl::OnLoad(mojom::VisitDataPtr visit_data,
                               uint64_t current_time) {
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

void RewardsEngineImpl::OnUnload(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  OnHide(tab_id, current_time);
  auto iter = current_pages_.find(tab_id);
  if (iter != current_pages_.end()) {
    current_pages_.erase(iter);
  }
}

void RewardsEngineImpl::OnShow(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  last_tab_active_time_ = current_time;
  last_shown_tab_id_ = tab_id;
}

void RewardsEngineImpl::OnHide(uint32_t tab_id, uint64_t current_time) {
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

void RewardsEngineImpl::OnForeground(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  // When performing automated testing, ignore changes in browser window
  // activation. When running tests in parallel, activation changes can
  // interfere with AC calculations on some platforms.
  if (options().is_testing) {
    return;
  }

  if (last_shown_tab_id_ != tab_id) {
    return;
  }

  OnShow(tab_id, current_time);
}

void RewardsEngineImpl::OnBackground(uint32_t tab_id, uint64_t current_time) {
  if (!IsReady()) {
    return;
  }

  // When performing automated testing, ignore changes in browser window
  // activation. When running tests in parallel, activation changes can
  // interfere with AC calculations on some platforms.
  if (options().is_testing) {
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
  if (!IsReady()) {
    return;
  }

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
  WhenReady(
      [this, publisher_key, exclude, callback = std::move(callback)]() mutable {
        publisher()->SetPublisherExclude(publisher_key, exclude,
                                         std::move(callback));
      });
}

void RewardsEngineImpl::RestorePublishers(RestorePublishersCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->RestorePublishers(std::move(callback));
  });
}

void RewardsEngineImpl::FetchPromotions(FetchPromotionsCallback callback) {
  // The promotion endpoint is no longer supported. The endpoint implementation,
  // the interface method, and all calling code will be removed when the
  // "grandfathered" vBAT state is removed from the codebase. Browser tests that
  // assume vBAT contributions will also need to be modified.
  if (!options().is_testing) {
    std::move(callback).Run(mojom::Result::OK, {});
    return;
  }

  WhenReady([this, callback = std::move(callback)]() mutable {
    promotion()->Fetch(std::move(callback));
  });
}

void RewardsEngineImpl::ClaimPromotion(const std::string& promotion_id,
                                       const std::string& payload,
                                       ClaimPromotionCallback callback) {
  WhenReady(
      [this, promotion_id, payload, callback = std::move(callback)]() mutable {
        promotion()->Claim(promotion_id, payload, std::move(callback));
      });
}

void RewardsEngineImpl::AttestPromotion(const std::string& promotion_id,
                                        const std::string& solution,
                                        AttestPromotionCallback callback) {
  WhenReady(
      [this, promotion_id, solution, callback = std::move(callback)]() mutable {
        promotion()->Attest(promotion_id, solution, std::move(callback));
      });
}

void RewardsEngineImpl::SetPublisherMinVisitTime(int duration_in_seconds) {
  WhenReady([this, duration_in_seconds]() {
    state()->SetPublisherMinVisitTime(duration_in_seconds);
  });
}

void RewardsEngineImpl::SetPublisherMinVisits(int visits) {
  WhenReady([this, visits]() { state()->SetPublisherMinVisits(visits); });
}

void RewardsEngineImpl::SetAutoContributionAmount(double amount) {
  WhenReady([this, amount]() { state()->SetAutoContributionAmount(amount); });
}

void RewardsEngineImpl::SetAutoContributeEnabled(bool enabled) {
  WhenReady([this, enabled]() { state()->SetAutoContributeEnabled(enabled); });
}

void RewardsEngineImpl::GetBalanceReport(mojom::ActivityMonth month,
                                         int32_t year,
                                         GetBalanceReportCallback callback) {
  WhenReady([this, month, year, callback = std::move(callback)]() mutable {
    database()->GetBalanceReportInfo(month, year, std::move(callback));
  });
}

void RewardsEngineImpl::GetPublisherActivityFromUrl(
    uint64_t window_id,
    mojom::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  WhenReady([this, window_id, visit_data = std::move(visit_data),
             publisher_blob]() mutable {
    publisher()->GetPublisherActivityFromUrl(window_id, std::move(visit_data),
                                             publisher_blob);
  });
}

void RewardsEngineImpl::GetAutoContributionAmount(
    GetAutoContributionAmountCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetAutoContributionAmount());
}

void RewardsEngineImpl::GetPublisherBanner(
    const std::string& publisher_id,
    GetPublisherBannerCallback callback) {
  WhenReady([this, publisher_id,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    publisher()->GetPublisherBanner(publisher_id, std::move(callback));
  });
}

void RewardsEngineImpl::OneTimeTip(const std::string& publisher_key,
                                   double amount,
                                   OneTimeTipCallback callback) {
  WhenReady([this, publisher_key, amount,
             callback = ToLegacyCallback(std::move(callback))]() {
    contribution()->OneTimeTip(publisher_key, amount, std::move(callback));
  });
}

void RewardsEngineImpl::RemoveRecurringTip(
    const std::string& publisher_key,
    RemoveRecurringTipCallback callback) {
  WhenReady([this, publisher_key,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->RemoveRecurringTip(publisher_key, std::move(callback));
  });
}

void RewardsEngineImpl::GetCreationStamp(GetCreationStampCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(state()->GetCreationStamp());
}

void RewardsEngineImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    auto info = mojom::RewardsInternalsInfo::New();

    mojom::RewardsWalletPtr rewards_wallet = wallet()->GetWallet();
    if (!rewards_wallet) {
      LogError(FROM_HERE) << "Wallet is null";
      std::move(callback).Run(std::move(info));
      return;
    }

    // Retrieve the payment id.
    info->payment_id = rewards_wallet->payment_id;

    // Retrieve the boot stamp.
    info->boot_stamp = state()->GetCreationStamp();

    // Retrieve the key info seed and validate it.
    if (Signer::FromRecoverySeed(rewards_wallet->recovery_seed)) {
      info->is_key_info_seed_valid = true;
    }

    std::move(callback).Run(std::move(info));
  });
}

void RewardsEngineImpl::SaveRecurringTip(mojom::RecurringTipPtr info,
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

void RewardsEngineImpl::SendContribution(const std::string& publisher_id,
                                         double amount,
                                         bool set_monthly,
                                         SendContributionCallback callback) {
  WhenReady([this, publisher_id, amount, set_monthly,
             callback = std::move(callback)]() mutable {
    contribution()->SendContribution(publisher_id, amount, set_monthly,
                                     std::move(callback));
  });
}

void RewardsEngineImpl::GetRecurringTips(GetRecurringTipsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    contribution()->GetRecurringTips(std::move(callback));
  });
}

void RewardsEngineImpl::GetOneTimeTips(GetOneTimeTipsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetOneTimeTips(util::GetCurrentMonth(), util::GetCurrentYear(),
                               std::move(callback));
  });
}

void RewardsEngineImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    mojom::ActivityInfoFilterPtr filter,
    GetActivityInfoListCallback callback) {
  WhenReady([this, start, limit, filter = std::move(filter),
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetActivityInfoList(start, limit, std::move(filter),
                                    std::move(callback));
  });
}

void RewardsEngineImpl::GetPublishersVisitedCount(
    GetPublishersVisitedCountCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->GetPublishersVisitedCount(std::move(callback));
  });
}

void RewardsEngineImpl::GetExcludedList(GetExcludedListCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetExcludedList(std::move(callback));
  });
}

void RewardsEngineImpl::RefreshPublisher(const std::string& publisher_key,
                                         RefreshPublisherCallback callback) {
  WhenReady([this, publisher_key,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    publisher()->RefreshPublisher(publisher_key, std::move(callback));
  });
}

void RewardsEngineImpl::StartContributionsForTesting() {
  WhenReady([this]() {
    contribution()->StartContributionsForTesting();  // IN-TEST
  });
}

void RewardsEngineImpl::UpdateMediaDuration(uint64_t window_id,
                                            const std::string& publisher_key,
                                            uint64_t duration,
                                            bool first_visit) {
  WhenReady([this, window_id, publisher_key, duration, first_visit]() {
    publisher()->UpdateMediaDuration(window_id, publisher_key, duration,
                                     first_visit);
  });
}

void RewardsEngineImpl::IsPublisherRegistered(
    const std::string& publisher_id,
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

void RewardsEngineImpl::GetPublisherInfo(const std::string& publisher_key,
                                         GetPublisherInfoCallback callback) {
  WhenReady([this, publisher_key,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetPublisherInfo(publisher_key, std::move(callback));
  });
}

void RewardsEngineImpl::GetPublisherPanelInfo(
    const std::string& publisher_key,
    GetPublisherPanelInfoCallback callback) {
  WhenReady([this, publisher_key,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    publisher()->GetPublisherPanelInfo(publisher_key, std::move(callback));
  });
}

void RewardsEngineImpl::SavePublisherInfo(
    uint64_t window_id,
    mojom::PublisherInfoPtr publisher_info,
    SavePublisherInfoCallback callback) {
  WhenReady([this, window_id, info = std::move(publisher_info),
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    publisher()->SavePublisherInfo(window_id, std::move(info),
                                   std::move(callback));
  });
}

void RewardsEngineImpl::GetShareURL(
    const base::flat_map<std::string, std::string>& args,
    GetShareURLCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run("");
  }

  std::move(callback).Run(publisher()->GetShareURL(args));
}

void RewardsEngineImpl::FetchBalance(FetchBalanceCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    wallet()->FetchBalance(std::move(callback));
  });
}

void RewardsEngineImpl::GetExternalWallet(GetExternalWalletCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    mojom::ExternalWalletPtr wallet;
    auto wallet_type = GetState<std::string>(state::kExternalWalletType);
    if (auto* provider = GetExternalWalletProvider(wallet_type)) {
      wallet = provider->GetWallet();
      if (wallet && (wallet->status == mojom::WalletStatus::kNotConnected)) {
        wallet = nullptr;
      }
    }
    std::move(callback).Run(std::move(wallet));
  });
}

void RewardsEngineImpl::BeginExternalWalletLogin(
    const std::string& wallet_type,
    BeginExternalWalletLoginCallback callback) {
  WhenReady([this, wallet_type, callback = std::move(callback)]() mutable {
    if (auto* provider = GetExternalWalletProvider(wallet_type)) {
      provider->BeginLogin(std::move(callback));
    } else {
      LogError(FROM_HERE) << "Invalid external wallet type";
      std::move(callback).Run(nullptr);
    }
  });
}

void RewardsEngineImpl::ConnectExternalWallet(
    const std::string& wallet_type,
    const base::flat_map<std::string, std::string>& args,
    ConnectExternalWalletCallback callback) {
  WhenReady([this, wallet_type, args,
             callback = std::move(callback)]() mutable {
    if (auto* provider = GetExternalWalletProvider(wallet_type)) {
      provider->ConnectWallet(args, std::move(callback));
    } else {
      LogError(FROM_HERE) << "Invalid external wallet type";
      std::move(callback).Run(mojom::ConnectExternalWalletResult::kUnexpected);
    }
  });
}

void RewardsEngineImpl::GetTransactionReport(
    mojom::ActivityMonth month,
    int year,
    GetTransactionReportCallback callback) {
  WhenReady([this, month, year,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetTransactionReport(month, year, std::move(callback));
  });
}

void RewardsEngineImpl::GetContributionReport(
    mojom::ActivityMonth month,
    int year,
    GetContributionReportCallback callback) {
  WhenReady([this, month, year,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetContributionReport(month, year, std::move(callback));
  });
}

void RewardsEngineImpl::GetAllContributions(
    GetAllContributionsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetAllContributions(std::move(callback));
  });
}

void RewardsEngineImpl::GetMonthlyReport(mojom::ActivityMonth month,
                                         int year,
                                         GetMonthlyReportCallback callback) {
  WhenReady([this, month, year,
             callback = ToLegacyCallback(std::move(callback))]() mutable {
    report()->GetMonthly(month, year, std::move(callback));
  });
}

void RewardsEngineImpl::GetAllMonthlyReportIds(
    GetAllMonthlyReportIdsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    report()->GetAllMonthlyIds(std::move(callback));
  });
}

void RewardsEngineImpl::GetAllPromotions(GetAllPromotionsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetAllPromotions(std::move(callback));
  });
}

void RewardsEngineImpl::Shutdown(ShutdownCallback callback) {
  Get<InitializationManager>().Shutdown(
      base::BindOnce(&RewardsEngineImpl::OnShutdownComplete,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void RewardsEngineImpl::GetEventLogs(GetEventLogsCallback callback) {
  WhenReady([this, callback = ToLegacyCallback(std::move(callback))]() mutable {
    database()->GetLastEventLogs(std::move(callback));
  });
}

void RewardsEngineImpl::GetRewardsWallet(GetRewardsWalletCallback callback) {
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

RewardsLogStream RewardsEngineImpl::Log(base::Location location) {
  return RewardsLogStream(*client_, location, 1);
}

RewardsLogStream RewardsEngineImpl::LogError(base::Location location) {
  return RewardsLogStream(*client_, location, 0);
}

std::optional<std::string> RewardsEngineImpl::EncryptString(
    const std::string& value) {
  std::optional<std::string> result;
  client_->EncryptString(value, &result);
  return result;
}

std::optional<std::string> RewardsEngineImpl::DecryptString(
    const std::string& value) {
  std::optional<std::string> result;
  client_->DecryptString(value, &result);
  return result;
}
// mojom::RewardsEngineClient helpers end

mojom::RewardsEngineClient* RewardsEngineImpl::client() {
  return client_.get();
}

database::Database* RewardsEngineImpl::database() {
  return database_.get();
}

wallet_provider::WalletProvider* RewardsEngineImpl::GetExternalWalletProvider(
    const std::string& wallet_type) {
  if (wallet_type == constant::kWalletBitflyer) {
    return bitflyer_.get();
  }
  if (wallet_type == constant::kWalletGemini) {
    return gemini_.get();
  }
  if (wallet_type == constant::kWalletUphold) {
    return uphold_.get();
  }
  if (wallet_type == constant::kWalletZebPay) {
    return zebpay_.get();
  }
  if (wallet_type == constant::kWalletSolana) {
    return &Get<SolanaWalletProvider>();
  }
  return nullptr;
}

bool RewardsEngineImpl::IsReady() const {
  return Get<InitializationManager>().is_ready();
}

void RewardsEngineImpl::OnInitializationComplete(InitializeCallback callback,
                                                 bool success) {
  ready_event_.Signal();
  std::move(callback).Run(success ? mojom::Result::OK : mojom::Result::FAILED);
}

void RewardsEngineImpl::OnShutdownComplete(ShutdownCallback callback,
                                           bool success) {
  std::move(callback).Run(success ? mojom::Result::OK : mojom::Result::FAILED);
}

template <typename T>
void RewardsEngineImpl::WhenReady(T callback) {
  switch (Get<InitializationManager>().state()) {
    case InitializationManager::State::kReady:
      callback();
      break;
    case InitializationManager::State::kShuttingDown:
      NOTREACHED();
      break;
    default:
      ready_event_.Post(
          FROM_HERE,
          base::BindOnce([](T callback) { callback(); }, std::move(callback)));
      break;
  }
}

}  // namespace brave_rewards::internal
