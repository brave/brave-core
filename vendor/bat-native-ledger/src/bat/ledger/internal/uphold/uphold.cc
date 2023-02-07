/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/endpoint/uphold/uphold_server.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
#include "bat/ledger/internal/wallet_provider/uphold/connect_uphold_wallet.h"
#include "bat/ledger/internal/wallet_provider/uphold/get_uphold_wallet.h"
#include "bat/ledger/internal/wallet_provider/uphold/uphold_transfer.h"
#include "brave_base/random.h"

namespace ledger::uphold {

Uphold::Uphold(LedgerImpl* ledger)
    : card_(std::make_unique<UpholdCard>(ledger)),
      connect_wallet_(std::make_unique<ConnectUpholdWallet>(ledger)),
      get_wallet_(std::make_unique<GetUpholdWallet>(ledger)),
      transfer_(std::make_unique<UpholdTransfer>(ledger)),
      uphold_server_(std::make_unique<endpoint::UpholdServer>(ledger)),
      ledger_(ledger) {}

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
                               ledger::LegacyResultCallback callback) {
  if (!info) {
    BLOG(0, "Publisher info is null");
    return callback(mojom::Result::LEDGER_ERROR);
  }

  const double fee = amount * 0.05;

  transfer_->Run(contribution_id, info->address, amount - fee,
                 base::BindOnce(&Uphold::ContributionCompleted,
                                base::Unretained(this), std::move(callback),
                                contribution_id, fee, info->publisher_key));
}

void Uphold::ContributionCompleted(ledger::LegacyResultCallback callback,
                                   const std::string& contribution_id,
                                   double fee,
                                   const std::string& publisher_key,
                                   mojom::Result result) {
  if (result == mojom::Result::LEDGER_OK) {
    SaveTransferFee(contribution_id, fee);

    if (!publisher_key.empty()) {
      ledger_->database()->UpdateContributionInfoContributedAmount(
          contribution_id, publisher_key, callback);
      return;
    }
  }

  callback(result);
}

void Uphold::FetchBalance(FetchBalanceCallback callback) {
  auto wallet = GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::LEDGER_OK, 0.0);
  }

  auto url_callback = base::BindOnce(
      &Uphold::OnFetchBalance, base::Unretained(this), std::move(callback));

  uphold_server_->get_card()->Request(wallet->address, wallet->token,
                                      std::move(url_callback));
}

void Uphold::OnFetchBalance(FetchBalanceCallback callback,
                            const mojom::Result result,
                            const double available) {
  if (!GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR, 0.0);
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    if (!LogOutWallet()) {
      BLOG(0, "Failed to disconnect " << constant::kWalletUphold << " wallet!");
      return std::move(callback).Run(mojom::Result::LEDGER_ERROR, 0.0);
    }

    return std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, 0.0);
  }

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get balance");
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR, 0.0);
  }

  std::move(callback).Run(mojom::Result::LEDGER_OK, available);
}

void Uphold::TransferFunds(double amount,
                           const std::string& address,
                           const std::string& contribution_id,
                           client::LegacyResultCallback callback) {
  transfer_->Run(contribution_id, address, amount,
                 base::BindOnce([](client::LegacyResultCallback callback,
                                   mojom::Result result) { callback(result); },
                                std::move(callback)));
}

void Uphold::ConnectWallet(const base::flat_map<std::string, std::string>& args,
                           ledger::ConnectExternalWalletCallback callback) {
  connect_wallet_->Run(args, std::move(callback));
}

void Uphold::GetWallet(ledger::GetExternalWalletCallback callback) {
  get_wallet_->Run(std::move(callback));
}

void Uphold::CreateCard(const std::string& access_token,
                        CreateCardCallback callback) {
  card_->CreateBATCardIfNecessary(access_token, std::move(callback));
}

void Uphold::GetUser(const std::string& access_token, GetMeCallback callback) {
  uphold_server_->get_me()->Request(access_token, std::move(callback));
}

void Uphold::GetCapabilities(const std::string& access_token,
                             GetCapabilitiesCallback callback) {
  uphold_server_->get_capabilities()->Request(access_token,
                                              std::move(callback));
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

void Uphold::TransferFee(const std::string& contribution_id,
                         double amount,
                         int attempts) {
  transfer_->Run(
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
  return ledger::wallet::GetWallet(ledger_, constant::kWalletUphold);
}

mojom::ExternalWalletPtr Uphold::GetWalletIf(
    const std::set<mojom::WalletStatus>& statuses) {
  return ledger::wallet::GetWalletIf(ledger_, constant::kWalletUphold,
                                     statuses);
}

bool Uphold::SetWallet(mojom::ExternalWalletPtr wallet) {
  return ledger::wallet::SetWallet(ledger_, std::move(wallet));
}

mojom::ExternalWalletPtr Uphold::TransitionWallet(
    mojom::ExternalWalletPtr wallet,
    mojom::WalletStatus to) {
  return ledger::wallet::TransitionWallet(ledger_, std::move(wallet), to);
}

bool Uphold::LogOutWallet(const std::string& notification) {
  return ledger::wallet::LogOutWallet(ledger_, constant::kWalletUphold,
                                      notification);
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

}  // namespace ledger::uphold
