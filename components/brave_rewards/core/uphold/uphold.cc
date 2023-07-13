/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/uphold/uphold.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/uuid.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/uphold_server.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/uphold/uphold_card.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"
#include "brave/components/brave_rewards/core/wallet_provider/uphold/connect_uphold_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/uphold/get_uphold_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/uphold/uphold_transfer.h"
#include "brave_base/random.h"

namespace brave_rewards::internal::uphold {

Uphold::Uphold(RewardsEngineImpl& engine)
    : engine_(engine),
      card_(engine),
      connect_wallet_(engine),
      get_wallet_(engine),
      transfer_(engine),
      uphold_server_(engine) {}

Uphold::~Uphold() = default;

void Uphold::Initialize() {
  auto wallet = GetWallet();
  if (!wallet) {
    return;
  }

  for (auto const& value : wallet->fees) {
    StartTransferFeeTimer(value.first, 1);
  }
}

void Uphold::StartContribution(const std::string& contribution_id,
                               mojom::ServerPublisherInfoPtr info,
                               double amount,
                               LegacyResultCallback callback) {
  if (!info) {
    BLOG(0, "Publisher info is null");
    return callback(mojom::Result::FAILED);
  }

  const double fee = amount * 0.05;

  transfer_.Run(contribution_id, info->address, amount - fee,
                base::BindOnce(&Uphold::ContributionCompleted,
                               base::Unretained(this), std::move(callback),
                               contribution_id, fee, info->publisher_key));
}

void Uphold::ContributionCompleted(LegacyResultCallback callback,
                                   const std::string& contribution_id,
                                   double fee,
                                   const std::string& publisher_key,
                                   mojom::Result result) {
  if (result == mojom::Result::OK) {
    SaveTransferFee(contribution_id, fee);

    if (!publisher_key.empty()) {
      engine_->database()->UpdateContributionInfoContributedAmount(
          contribution_id, publisher_key, callback);
      return;
    }
  }

  callback(result);
}

void Uphold::FetchBalance(FetchBalanceCallback callback) {
  auto wallet = GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  auto url_callback = base::BindOnce(
      &Uphold::OnFetchBalance, base::Unretained(this), std::move(callback));

  uphold_server_.get_card().Request(wallet->address, wallet->token,
                                    std::move(url_callback));
}

void Uphold::OnFetchBalance(FetchBalanceCallback callback,
                            const mojom::Result result,
                            const double available) {
  if (!GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    if (!LogOutWallet()) {
      BLOG(0, "Failed to disconnect " << constant::kWalletUphold << " wallet!");
      return std::move(callback).Run(mojom::Result::FAILED, 0.0);
    }

    return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, 0.0);
  }

  if (result != mojom::Result::OK) {
    BLOG(0, "Couldn't get balance");
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  std::move(callback).Run(mojom::Result::OK, available);
}

void Uphold::TransferFunds(double amount,
                           const std::string& address,
                           const std::string& contribution_id,
                           LegacyResultCallback callback) {
  transfer_.Run(contribution_id, address, amount,
                base::BindOnce([](LegacyResultCallback callback,
                                  mojom::Result result) { callback(result); },
                               std::move(callback)));
}

void Uphold::ConnectWallet(const base::flat_map<std::string, std::string>& args,
                           ConnectExternalWalletCallback callback) {
  connect_wallet_.Run(args, std::move(callback));
}

void Uphold::GetWallet(GetExternalWalletCallback callback) {
  get_wallet_.Run(std::move(callback));
}

void Uphold::CreateCard(const std::string& access_token,
                        CreateCardCallback callback) {
  card_.CreateBATCardIfNecessary(access_token, std::move(callback));
}

void Uphold::GetUser(const std::string& access_token, GetMeCallback callback) {
  uphold_server_.get_me().Request(access_token, std::move(callback));
}

void Uphold::GetCapabilities(const std::string& access_token,
                             GetCapabilitiesCallback callback) {
  uphold_server_.get_capabilities().Request(access_token, std::move(callback));
}

void Uphold::SaveTransferFee(const std::string& contribution_id,
                             const double fee) {
  StartTransferFeeTimer(contribution_id, 1);

  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  wallet->fees.insert(std::make_pair(contribution_id, fee));
  if (!SetWallet(std::move(wallet))) {
    BLOG(0, "Failed to set " << constant::kWalletUphold << " wallet!");
  }
}

void Uphold::StartTransferFeeTimer(const std::string& fee_id, int attempts) {
  DCHECK(!fee_id.empty());

  base::TimeDelta delay = util::GetRandomizedDelay(base::Seconds(45));

  BLOG(1, "Uphold transfer fee timer set for " << delay);

  transfer_fee_timers_[fee_id].Start(
      FROM_HERE, delay,
      base::BindOnce(&Uphold::OnTransferFeeTimerElapsed, base::Unretained(this),
                     fee_id, attempts));
}

void Uphold::OnTransferFeeCompleted(const std::string& contribution_id,
                                    int attempts,
                                    mojom::Result result) {
  if (result != mojom::Result::OK) {
    if (attempts < 3) {
      BLOG(0, "Transaction fee failed, retrying");
      StartTransferFeeTimer(contribution_id, attempts + 1);
      return;
    }
    BLOG(0, "Transaction fee failed, no remaining attempts this session");
    return;
  }

  RemoveTransferFee(contribution_id);
}

void Uphold::TransferFee(const std::string& contribution_id,
                         double amount,
                         int attempts) {
  transfer_.Run(
      contribution_id, GetFeeAddress(), amount,
      base::BindOnce(&Uphold::OnTransferFeeCompleted, base::Unretained(this),
                     contribution_id, attempts));
}

void Uphold::OnTransferFeeTimerElapsed(const std::string& id, int attempts) {
  transfer_fee_timers_.erase(id);

  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  for (const auto& value : wallet->fees) {
    if (value.first == id) {
      TransferFee(value.first, value.second, attempts);
      return;
    }
  }
}

mojom::ExternalWalletPtr Uphold::GetWallet() {
  return wallet::GetWallet(*engine_, constant::kWalletUphold);
}

mojom::ExternalWalletPtr Uphold::GetWalletIf(
    const std::set<mojom::WalletStatus>& statuses) {
  return wallet::GetWalletIf(*engine_, constant::kWalletUphold, statuses);
}

bool Uphold::SetWallet(mojom::ExternalWalletPtr wallet) {
  return wallet::SetWallet(*engine_, std::move(wallet));
}

mojom::ExternalWalletPtr Uphold::TransitionWallet(
    mojom::ExternalWalletPtr wallet,
    mojom::WalletStatus to) {
  return wallet::TransitionWallet(*engine_, std::move(wallet), to);
}

bool Uphold::LogOutWallet(const std::string& notification) {
  return wallet::LogOutWallet(*engine_, constant::kWalletUphold, notification);
}

void Uphold::RemoveTransferFee(const std::string& contribution_id) {
  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  wallet->fees.erase(contribution_id);
  if (!SetWallet(std::move(wallet))) {
    BLOG(0, "Failed to set " << constant::kWalletUphold << " wallet!");
  }
}

}  // namespace brave_rewards::internal::uphold
