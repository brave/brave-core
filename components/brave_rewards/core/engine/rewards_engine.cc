/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/rewards_engine.h"

#include <vector>

#include "base/check.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/components/brave_rewards/core/engine/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/engine/contribution/contribution.h"
#include "brave/components/brave_rewards/core/engine/database/database.h"
#include "brave/components/brave_rewards/core/engine/endpoints/brave/get_ui_cards.h"
#include "brave/components/brave_rewards/core/engine/gemini/gemini.h"
#include "brave/components/brave_rewards/core/engine/global_constants.h"
#include "brave/components/brave_rewards/core/engine/initialization_manager.h"
#include "brave/components/brave_rewards/core/engine/parameters/rewards_parameters_provider.h"
#include "brave/components/brave_rewards/core/engine/publisher/media/media.h"
#include "brave/components/brave_rewards/core/engine/publisher/publisher.h"
#include "brave/components/brave_rewards/core/engine/publisher/static_values.h"
#include "brave/components/brave_rewards/core/engine/uphold/uphold.h"
#include "brave/components/brave_rewards/core/engine/util/environment_config.h"
#include "brave/components/brave_rewards/core/engine/util/rewards_prefs.h"
#include "brave/components/brave_rewards/core/engine/util/signer.h"
#include "brave/components/brave_rewards/core/engine/util/time_util.h"
#include "brave/components/brave_rewards/core/engine/util/url_loader.h"
#include "brave/components/brave_rewards/core/engine/wallet/wallet.h"
#include "brave/components/brave_rewards/core/engine/wallet_provider/solana/solana_wallet_provider.h"
#include "brave/components/brave_rewards/core/engine/wallet_provider/wallet_provider.h"
#include "brave/components/brave_rewards/core/engine/zebpay/zebpay.h"

namespace brave_rewards::internal {

RewardsEngine::RewardsEngine(
    mojo::PendingAssociatedRemote<mojom::RewardsEngineClient> client_remote,
    const mojom::RewardsEngineOptions& options)
    : client_(std::move(client_remote)),
      options_(options),
      publisher_(std::make_unique<publisher::Publisher>(*this)),
      media_(std::make_unique<Media>(*this)),
      contribution_(std::make_unique<contribution::Contribution>(*this)),
      wallet_(std::make_unique<wallet::Wallet>(*this)),
      database_(std::make_unique<database::Database>(*this)),
      bitflyer_(std::make_unique<bitflyer::Bitflyer>(*this)),
      gemini_(std::make_unique<gemini::Gemini>(*this)),
      uphold_(std::make_unique<uphold::Uphold>(*this)),
      zebpay_(std::make_unique<zebpay::ZebPay>(*this)) {
  DCHECK(base::ThreadPoolInstance::Get());
}

RewardsEngine::~RewardsEngine() = default;

// mojom::RewardsEngine implementation begin (in the order of appearance in
// Mojom)
void RewardsEngine::Initialize(InitializeCallback callback) {
  Get<InitializationManager>().Initialize(
      base::BindOnce(&RewardsEngine::OnInitializationComplete, GetWeakPtr(),
                     std::move(callback)));
}

void RewardsEngine::GetEnvironment(GetEnvironmentCallback callback) {
  std::move(callback).Run(Get<EnvironmentConfig>().current_environment());
}

void RewardsEngine::CreateRewardsWallet(const std::string& country,
                                        CreateRewardsWalletCallback callback) {
  WhenReady([this, country, callback = std::move(callback)]() mutable {
    wallet()->CreateWalletIfNecessary(
        country.empty() ? std::nullopt
                        : std::optional<std::string>(std::move(country)),
        std::move(callback));
  });
}

void RewardsEngine::GetRewardsParameters(
    GetRewardsParametersCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    Get<RewardsParametersProvider>().GetParameters(std::move(callback));
  });
}

void RewardsEngine::FetchUICards(FetchUICardsCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    Get<endpoints::GetUICards>().Request(std::move(callback));
  });
}

void RewardsEngine::GetPublisherMinVisitTime(
    GetPublisherMinVisitTimeCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(Get<RewardsPrefs>().GetInteger(prefs::kMinVisitTime));
}

void RewardsEngine::GetPublisherMinVisits(
    GetPublisherMinVisitsCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(Get<RewardsPrefs>().GetInteger(prefs::kMinVisits));
}

void RewardsEngine::GetReconcileStamp(GetReconcileStampCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(contribution()->GetReconcileStamp());
}

void RewardsEngine::SetPublisherExclude(const std::string& publisher_key,
                                        mojom::PublisherExclude exclude,
                                        SetPublisherExcludeCallback callback) {
  WhenReady(
      [this, publisher_key, exclude, callback = std::move(callback)]() mutable {
        publisher()->SetPublisherExclude(publisher_key, exclude,
                                         std::move(callback));
      });
}

void RewardsEngine::RestorePublishers(RestorePublishersCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->RestorePublishers(std::move(callback));
  });
}

void RewardsEngine::SetPublisherMinVisitTime(int duration_in_seconds) {
  WhenReady([this, duration_in_seconds] {
    Get<RewardsPrefs>().SetInteger(prefs::kMinVisitTime, duration_in_seconds);
    publisher()->CalcScoreConsts(duration_in_seconds);
    publisher()->SynopsisNormalizer();
  });
}

void RewardsEngine::SetPublisherMinVisits(int visits) {
  WhenReady([this, visits] {
    Get<RewardsPrefs>().SetInteger(prefs::kMinVisits, visits);
    publisher()->SynopsisNormalizer();
  });
}

void RewardsEngine::GetBalanceReport(mojom::ActivityMonth month,
                                     int32_t year,
                                     GetBalanceReportCallback callback) {
  WhenReady([this, month, year, callback = std::move(callback)]() mutable {
    database()->GetBalanceReportInfo(month, year, std::move(callback));
  });
}

void RewardsEngine::NotifyPublisherPageVisit(
    uint64_t tab_id,
    mojom::VisitDataPtr visit_data,
    const std::string& publisher_blob) {
  WhenReady([this, tab_id, visit_data = std::move(visit_data),
             publisher_blob]() mutable {
    publisher()->NotifyPublisherPageVisit(tab_id, std::move(visit_data),
                                          publisher_blob);
  });
}

void RewardsEngine::GetPublisherBanner(const std::string& publisher_id,
                                       GetPublisherBannerCallback callback) {
  WhenReady([this, publisher_id, callback = std::move(callback)]() mutable {
    publisher()->GetPublisherBanner(publisher_id, std::move(callback));
  });
}

void RewardsEngine::OneTimeTip(const std::string& publisher_key,
                               double amount,
                               OneTimeTipCallback callback) {
  WhenReady(
      [this, publisher_key, amount, callback = std::move(callback)]() mutable {
        contribution()->OneTimeTip(publisher_key, amount, std::move(callback));
      });
}

void RewardsEngine::RemoveRecurringTip(const std::string& publisher_key,
                                       RemoveRecurringTipCallback callback) {
  WhenReady([this, publisher_key, callback = std::move(callback)]() mutable {
    database()->RemoveRecurringTip(publisher_key, std::move(callback));
  });
}

void RewardsEngine::GetCreationStamp(GetCreationStampCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run(0);
  }

  std::move(callback).Run(Get<RewardsPrefs>().GetUint64(prefs::kCreationStamp));
}

void RewardsEngine::GetRewardsInternalsInfo(
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
    info->boot_stamp = Get<RewardsPrefs>().GetUint64(prefs::kCreationStamp);

    // Retrieve the key info seed and validate it.
    if (Signer::FromRecoverySeed(rewards_wallet->recovery_seed)) {
      info->is_key_info_seed_valid = true;
    }

    std::move(callback).Run(std::move(info));
  });
}

void RewardsEngine::SaveRecurringTip(mojom::RecurringTipPtr info,
                                     SaveRecurringTipCallback callback) {
  WhenReady(
      [this, info = std::move(info), callback = std::move(callback)]() mutable {
        database()->SaveRecurringTip(
            std::move(info),
            base::BindOnce(&RewardsEngine::OnRecurringTipSaved,
                           weak_factory_.GetWeakPtr(), std::move(callback)));
      });
}

void RewardsEngine::OnRecurringTipSaved(SaveRecurringTipCallback callback,
                                        mojom::Result result) {
  contribution()->SetMonthlyContributionTimer();
  std::move(callback).Run(result);
}

void RewardsEngine::SendContribution(const std::string& publisher_id,
                                     double amount,
                                     bool set_monthly,
                                     SendContributionCallback callback) {
  WhenReady([this, publisher_id, amount, set_monthly,
             callback = std::move(callback)]() mutable {
    contribution()->SendContribution(publisher_id, amount, set_monthly,
                                     std::move(callback));
  });
}

void RewardsEngine::GetRecurringTips(GetRecurringTipsCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    contribution()->GetRecurringTips(std::move(callback));
  });
}

void RewardsEngine::GetOneTimeTips(GetOneTimeTipsCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->GetOneTimeTips(util::GetCurrentMonth(), util::GetCurrentYear(),
                               std::move(callback));
  });
}

void RewardsEngine::GetActivityInfoList(uint32_t start,
                                        uint32_t limit,
                                        mojom::ActivityInfoFilterPtr filter,
                                        GetActivityInfoListCallback callback) {
  WhenReady([this, start, limit, filter = std::move(filter),
             callback = std::move(callback)]() mutable {
    database()->GetActivityInfoList(start, limit, std::move(filter),
                                    std::move(callback));
  });
}

void RewardsEngine::GetPublishersVisitedCount(
    GetPublishersVisitedCountCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->GetPublishersVisitedCount(std::move(callback));
  });
}

void RewardsEngine::GetExcludedList(GetExcludedListCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->GetExcludedList(std::move(callback));
  });
}

void RewardsEngine::RefreshPublisher(const std::string& publisher_key,
                                     RefreshPublisherCallback callback) {
  WhenReady([this, publisher_key, callback = std::move(callback)]() mutable {
    publisher()->RefreshPublisher(publisher_key, std::move(callback));
  });
}

void RewardsEngine::StartContributionsForTesting() {
  WhenReady([this] {
    contribution()->StartContributionsForTesting();  // IN-TEST
  });
}

void RewardsEngine::IsPublisherRegistered(
    const std::string& publisher_id,
    IsPublisherRegisteredCallback callback) {
  WhenReady([this, publisher_id, callback = std::move(callback)]() mutable {
    publisher()->GetServerPublisherInfo(
        publisher_id, true /* use_prefix_list */,
        base::BindOnce(
            [](IsPublisherRegisteredCallback callback,
               mojom::ServerPublisherInfoPtr info) {
              std::move(callback).Run(
                  info && info->status != mojom::PublisherStatus::NOT_VERIFIED);
            },
            std::move(callback)));
  });
}

void RewardsEngine::GetPublisherInfo(const std::string& publisher_key,
                                     GetPublisherInfoCallback callback) {
  WhenReady([this, publisher_key, callback = std::move(callback)]() mutable {
    database()->GetPublisherInfo(publisher_key, std::move(callback));
  });
}

void RewardsEngine::GetPublisherPanelInfo(
    const std::string& publisher_key,
    GetPublisherPanelInfoCallback callback) {
  WhenReady([this, publisher_key, callback = std::move(callback)]() mutable {
    publisher()->GetPublisherPanelInfo(publisher_key, std::move(callback));
  });
}

void RewardsEngine::SavePublisherInfo(uint64_t window_id,
                                      mojom::PublisherInfoPtr publisher_info,
                                      SavePublisherInfoCallback callback) {
  WhenReady([this, window_id, info = std::move(publisher_info),
             callback = std::move(callback)]() mutable {
    publisher()->SavePublisherInfo(window_id, std::move(info),
                                   std::move(callback));
  });
}

void RewardsEngine::GetShareURL(
    const base::flat_map<std::string, std::string>& args,
    GetShareURLCallback callback) {
  if (!IsReady()) {
    return std::move(callback).Run("");
  }

  std::move(callback).Run(publisher()->GetShareURL(args));
}

void RewardsEngine::FetchBalance(FetchBalanceCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    wallet()->FetchBalance(std::move(callback));
  });
}

void RewardsEngine::GetExternalWallet(GetExternalWalletCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    mojom::ExternalWalletPtr wallet;
    const auto& wallet_type =
        Get<RewardsPrefs>().GetString(prefs::kExternalWalletType);
    if (auto* provider = GetExternalWalletProvider(wallet_type)) {
      wallet = provider->GetWallet();
      if (wallet && (wallet->status == mojom::WalletStatus::kNotConnected)) {
        wallet = nullptr;
      }
    }
    std::move(callback).Run(std::move(wallet));
  });
}

void RewardsEngine::BeginExternalWalletLogin(
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

void RewardsEngine::ConnectExternalWallet(
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

void RewardsEngine::GetAllContributions(GetAllContributionsCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->GetAllContributions(std::move(callback));
  });
}

void RewardsEngine::Shutdown(ShutdownCallback callback) {
  Get<InitializationManager>().Shutdown(base::BindOnce(
      &RewardsEngine::OnShutdownComplete, GetWeakPtr(), std::move(callback)));
}

void RewardsEngine::GetEventLogs(GetEventLogsCallback callback) {
  WhenReady([this, callback = std::move(callback)]() mutable {
    database()->GetLastEventLogs(std::move(callback));
  });
}

void RewardsEngine::GetRewardsWallet(GetRewardsWalletCallback callback) {
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

RewardsLogStream RewardsEngine::Log(base::Location location) {
  return RewardsLogStream(*client_, location, 1);
}

RewardsLogStream RewardsEngine::LogError(base::Location location) {
  return RewardsLogStream(*client_, location, 0);
}

base::WeakPtr<RewardsEngine> RewardsEngine::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

base::WeakPtr<const RewardsEngine> RewardsEngine::GetWeakPtr() const {
  return weak_factory_.GetWeakPtr();
}

mojom::RewardsEngineClient* RewardsEngine::client() {
  return client_.get();
}

database::Database* RewardsEngine::database() {
  return database_.get();
}

wallet_provider::WalletProvider* RewardsEngine::GetExternalWalletProvider(
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

bool RewardsEngine::IsReady() {
  return Get<InitializationManager>().is_ready();
}

void RewardsEngine::OnInitializationComplete(InitializeCallback callback,
                                             bool success) {
  ready_event_.Signal();
  std::move(callback).Run(success ? mojom::Result::OK : mojom::Result::FAILED);
}

void RewardsEngine::OnShutdownComplete(ShutdownCallback callback,
                                       bool success) {
  std::move(callback).Run(success ? mojom::Result::OK : mojom::Result::FAILED);
}

template <typename T>
void RewardsEngine::WhenReady(T callback) {
  switch (Get<InitializationManager>().state()) {
    case InitializationManager::State::kReady:
      callback();
      break;
    default:
      ready_event_.Post(
          FROM_HERE,
          base::BindOnce([](T callback) { callback(); }, std::move(callback)));
      break;
  }
}

}  // namespace brave_rewards::internal
