/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/wallet_provider.h"

#include <utility>

#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"

namespace brave_rewards::internal::wallet_provider {

WalletProvider::WalletProvider(RewardsEngine& engine) : engine_(engine) {}

WalletProvider::~WalletProvider() = default;

base::TimeDelta WalletProvider::GetDelay() const {
  return util::GetRandomizedDelay(base::Seconds(45));
}

void WalletProvider::OnWalletLinked(const std::string& address) {}

void WalletProvider::Initialize() {
  if (auto wallet = GetWallet()) {
    for (const auto& value : wallet->fees) {
      StartTransferFeeTimer(value.first, 1);
    }
  }
}

void WalletProvider::StartContribution(const std::string& contribution_id,
                                       mojom::ServerPublisherInfoPtr info,
                                       double amount,
                                       ResultCallback callback) {
  if (!transfer_) {
    engine_->LogError(FROM_HERE)
        << WalletType() << " does not support contributions";
    return std::move(callback).Run(mojom::Result::FAILED);
  }

  if (!info) {
    engine_->LogError(FROM_HERE) << "Publisher info is null";
    return std::move(callback).Run(mojom::Result::FAILED);
  }

  const double fee = amount * 0.05;

  transfer_->Run(contribution_id, info->address, amount - fee,
                 base::BindOnce(&WalletProvider::ContributionCompleted,
                                base::Unretained(this), std::move(callback),
                                contribution_id, fee, info->publisher_key));
}

void WalletProvider::ContributionCompleted(ResultCallback callback,
                                           const std::string& contribution_id,
                                           double fee,
                                           const std::string& publisher_key,
                                           mojom::Result result) {
  if (result == mojom::Result::OK) {
    SaveTransferFee(contribution_id, fee);

    if (!publisher_key.empty()) {
      return engine_->database()->UpdateContributionInfoContributedAmount(
          contribution_id, publisher_key, std::move(callback));
    }
  }

  std::move(callback).Run(result);
}

void WalletProvider::OnFetchBalance(
    base::OnceCallback<void(mojom::Result, double)> callback,
    mojom::Result result,
    double available) {
  if (!GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    engine_->LogError(FROM_HERE) << "Access token expired";
    if (!LogOutWallet()) {
      engine_->LogError(FROM_HERE)
          << "Failed to disconnect " << WalletType() << " wallet";
      return std::move(callback).Run(mojom::Result::FAILED, 0.0);
    }

    return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, 0.0);
  }

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE)
        << "Failed to get " << WalletType() << " balance";
    return std::move(callback).Run(mojom::Result::FAILED, 0.0);
  }

  std::move(callback).Run(mojom::Result::OK, available);
}

void WalletProvider::BeginLogin(BeginExternalWalletLoginCallback callback) {
  CHECK(connect_wallet_);
  auto params = mojom::ExternalWalletLoginParams::New();
  params->url = connect_wallet_->GenerateLoginURL();
  std::move(callback).Run(std::move(params));
}

void WalletProvider::ConnectWallet(
    const base::flat_map<std::string, std::string>& args,
    ConnectExternalWalletCallback callback) {
  // The connect wallet flow currently requires an ExternalWallet structure.
  // Create one now if necessary.
  wallet::MaybeCreateWallet(*engine_, WalletType());

  CHECK(connect_wallet_);
  connect_wallet_->Run(args, std::move(callback));
}

void WalletProvider::SaveTransferFee(const std::string& contribution_id,
                                     double fee) {
  StartTransferFeeTimer(contribution_id, 1);

  auto wallet = GetWallet();
  if (!wallet) {
    engine_->LogError(FROM_HERE) << WalletType() << " wallet is null";
    return;
  }

  wallet->fees.insert(std::make_pair(contribution_id, fee));
  if (!SetWallet(std::move(wallet))) {
    engine_->LogError(FROM_HERE)
        << "Failed to set " << WalletType() << " wallet";
  }
}

void WalletProvider::StartTransferFeeTimer(const std::string& fee_id,
                                           int attempts) {
  DCHECK(!fee_id.empty());

  base::TimeDelta delay = GetDelay();

  engine_->Log(FROM_HERE) << WalletType()
                          << " transfer fee timer is being set for " << delay;

  transfer_fee_timers_[fee_id].Start(
      FROM_HERE, delay,
      base::BindOnce(&WalletProvider::OnTransferFeeTimerElapsed,
                     base::Unretained(this), fee_id, attempts));
}

void WalletProvider::OnTransferFeeCompleted(const std::string& contribution_id,
                                            int attempts,
                                            mojom::Result result) {
  if (result != mojom::Result::OK) {
    if (attempts < 3) {
      engine_->LogError(FROM_HERE) << "Transaction fee failed, retrying";
      StartTransferFeeTimer(contribution_id, attempts + 1);
      return;
    }

    engine_->LogError(FROM_HERE)
        << "Transaction fee failed, no remaining attempts this session";
    return;
  }

  RemoveTransferFee(contribution_id);
}

void WalletProvider::TransferFee(const std::string& contribution_id,
                                 double amount,
                                 int attempts) {
  if (!transfer_) {
    engine_->LogError(FROM_HERE)
        << WalletType() << " does not support contributions";
    return;
  }

  transfer_->Run(
      contribution_id, GetFeeAddress(), amount,
      base::BindOnce(&WalletProvider::OnTransferFeeCompleted,
                     base::Unretained(this), contribution_id, attempts));
}

void WalletProvider::OnTransferFeeTimerElapsed(const std::string& id,
                                               int attempts) {
  transfer_fee_timers_.erase(id);

  auto wallet = GetWallet();
  if (!wallet) {
    engine_->LogError(FROM_HERE) << WalletType() << " wallet is null";
    return;
  }

  for (const auto& value : wallet->fees) {
    if (value.first == id) {
      TransferFee(value.first, value.second, attempts);
      return;
    }
  }
}

mojom::ExternalWalletPtr WalletProvider::GetWallet() {
  return wallet::GetWallet(*engine_, WalletType());
}

mojom::ExternalWalletPtr WalletProvider::GetWalletIf(
    const std::set<mojom::WalletStatus>& statuses) {
  return wallet::GetWalletIf(*engine_, WalletType(), statuses);
}

bool WalletProvider::SetWallet(mojom::ExternalWalletPtr wallet) {
  return wallet::SetWallet(*engine_, std::move(wallet));
}

bool WalletProvider::LogOutWallet(const std::string& notification) {
  return wallet::LogOutWallet(*engine_, WalletType(), notification);
}

void WalletProvider::RemoveTransferFee(const std::string& contribution_id) {
  auto wallet = GetWallet();
  if (!wallet) {
    engine_->LogError(FROM_HERE) << WalletType() << " wallet is null";
    return;
  }

  wallet->fees.erase(contribution_id);
  if (!SetWallet(std::move(wallet))) {
    engine_->LogError(FROM_HERE)
        << "Failed to set " << WalletType() << " wallet";
  }
}

}  // namespace brave_rewards::internal::wallet_provider
