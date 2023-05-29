/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/endpoint/bitflyer/bitflyer_server.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/logging/logging.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/wallet/wallet_util.h"
#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/bitflyer_transfer.h"
#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/connect_bitflyer_wallet.h"
#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/get_bitflyer_wallet.h"
#include "brave_base/random.h"

namespace brave_rewards::internal::bitflyer {

Bitflyer::Bitflyer() = default;

Bitflyer::~Bitflyer() = default;

void Bitflyer::Initialize() {
  auto wallet = GetWallet();
  if (!wallet) {
    return;
  }

  for (const auto& value : wallet->fees) {
    StartTransferFeeTimer(value.first, 1);
  }
}

void Bitflyer::StartContribution(const std::string& contribution_id,
                                 mojom::ServerPublisherInfoPtr info,
                                 double amount,
                                 LegacyResultCallback callback) {
  if (!info) {
    BLOG(0, "Publisher info is null");
    return callback(mojom::Result::LEDGER_ERROR);
  }

  const double fee = amount * 0.05;

  transfer_.Run(contribution_id, info->address, amount - fee,
                base::BindOnce(&Bitflyer::ContributionCompleted,
                               base::Unretained(this), std::move(callback),
                               contribution_id, fee, info->publisher_key));
}

void Bitflyer::ContributionCompleted(LegacyResultCallback callback,
                                     const std::string& contribution_id,
                                     double fee,
                                     const std::string& publisher_key,
                                     mojom::Result result) {
  if (result == mojom::Result::LEDGER_OK) {
    SaveTransferFee(contribution_id, fee);

    if (!publisher_key.empty()) {
      ledger().database()->UpdateContributionInfoContributedAmount(
          contribution_id, publisher_key, callback);
      return;
    }
  }

  callback(result);
}

void Bitflyer::FetchBalance(FetchBalanceCallback callback) {
  auto wallet = GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR, 0.0);
  }

  auto url_callback = base::BindOnce(
      &Bitflyer::OnFetchBalance, base::Unretained(this), std::move(callback));

  bitflyer_server_.get_balance().Request(wallet->token,
                                         std::move(url_callback));
}

void Bitflyer::OnFetchBalance(FetchBalanceCallback callback,
                              const mojom::Result result,
                              const double available) {
  if (!GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR, 0.0);
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    if (!LogOutWallet()) {
      BLOG(0,
           "Failed to disconnect " << constant::kWalletBitflyer << " wallet!");
      return std::move(callback).Run(mojom::Result::LEDGER_ERROR, 0.0);
    }

    return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, 0.0);
  }

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get balance");
    std::move(callback).Run(mojom::Result::LEDGER_ERROR, 0.0);
    return;
  }

  std::move(callback).Run(mojom::Result::LEDGER_OK, available);
}

void Bitflyer::TransferFunds(double amount,
                             const std::string& address,
                             const std::string& contribution_id,
                             LegacyResultCallback callback) {
  transfer_.Run(contribution_id, address, amount,
                base::BindOnce([](LegacyResultCallback callback,
                                  mojom::Result result) { callback(result); },
                               std::move(callback)));
}

void Bitflyer::ConnectWallet(
    const base::flat_map<std::string, std::string>& args,
    ConnectExternalWalletCallback callback) {
  connect_wallet_.Run(args, std::move(callback));
}

void Bitflyer::GetWallet(GetExternalWalletCallback callback) {
  get_wallet_.Run(std::move(callback));
}

void Bitflyer::SaveTransferFee(const std::string& contribution_id,
                               const double fee) {
  StartTransferFeeTimer(contribution_id, 1);

  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  wallet->fees.insert(std::make_pair(contribution_id, fee));
  if (!SetWallet(std::move(wallet))) {
    BLOG(0, "Failed to set " << constant::kWalletBitflyer << " wallet!");
  }
}

void Bitflyer::StartTransferFeeTimer(const std::string& fee_id,
                                     const int attempts) {
  DCHECK(!fee_id.empty());

  base::TimeDelta delay = util::GetRandomizedDelay(base::Seconds(45));

  BLOG(1, "Bitflyer transfer fee timer set for " << delay);

  transfer_fee_timers_[fee_id].Start(
      FROM_HERE, delay,
      base::BindOnce(&Bitflyer::OnTransferFeeTimerElapsed,
                     base::Unretained(this), fee_id, attempts));
}

void Bitflyer::OnTransferFeeCompleted(const std::string& contribution_id,
                                      int attempts,
                                      mojom::Result result) {
  if (result != mojom::Result::LEDGER_OK) {
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

void Bitflyer::TransferFee(const std::string& contribution_id,
                           double amount,
                           const int attempts) {
  transfer_.Run(
      contribution_id, GetFeeAddress(), amount,
      base::BindOnce(&Bitflyer::OnTransferFeeCompleted, base::Unretained(this),
                     contribution_id, attempts));
}

void Bitflyer::OnTransferFeeTimerElapsed(const std::string& id,
                                         const int attempts) {
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

mojom::ExternalWalletPtr Bitflyer::GetWallet() {
  return wallet::GetWallet(constant::kWalletBitflyer);
}

mojom::ExternalWalletPtr Bitflyer::GetWalletIf(
    const std::set<mojom::WalletStatus>& statuses) {
  return wallet::GetWalletIf(constant::kWalletBitflyer, statuses);
}

bool Bitflyer::SetWallet(mojom::ExternalWalletPtr wallet) {
  return wallet::SetWallet(std::move(wallet));
}

bool Bitflyer::LogOutWallet() {
  return wallet::LogOutWallet(constant::kWalletBitflyer);
}

void Bitflyer::RemoveTransferFee(const std::string& contribution_id) {
  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  wallet->fees.erase(contribution_id);
  if (!SetWallet(std::move(wallet))) {
    BLOG(0, "Failed to set " << constant::kWalletBitflyer << " wallet!");
  }
}

}  // namespace brave_rewards::internal::bitflyer
