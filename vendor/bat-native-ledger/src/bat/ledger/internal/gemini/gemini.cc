/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/endpoint/gemini/gemini_server.h"
#include "bat/ledger/internal/gemini/gemini.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
#include "bat/ledger/internal/wallet_provider/gemini/connect_gemini_wallet.h"
#include "bat/ledger/internal/wallet_provider/gemini/gemini_transfer.h"
#include "bat/ledger/internal/wallet_provider/gemini/get_gemini_wallet.h"
#include "brave_base/random.h"

namespace ledger::gemini {

Gemini::Gemini(LedgerImpl* ledger)
    : connect_wallet_(std::make_unique<ConnectGeminiWallet>(ledger)),
      get_wallet_(std::make_unique<GetGeminiWallet>(ledger)),
      transfer_(std::make_unique<GeminiTransfer>(ledger)),
      gemini_server_(std::make_unique<endpoint::GeminiServer>(ledger)),
      ledger_(ledger) {}

Gemini::~Gemini() = default;

void Gemini::Initialize() {
  auto wallet = GetWallet();
  if (!wallet) {
    return;
  }

  for (const auto& value : wallet->fees) {
    StartTransferFeeTimer(value.first, 1);
  }
}

void Gemini::StartContribution(const std::string& contribution_id,
                               mojom::ServerPublisherInfoPtr info,
                               double amount,
                               LegacyResultCallback callback) {
  if (!info) {
    BLOG(0, "Publisher info is null");
    return callback(mojom::Result::LEDGER_ERROR);
  }

  const double fee = amount * 0.05;

  transfer_->Run(contribution_id, info->address, amount - fee,
                 base::BindOnce(&Gemini::ContributionCompleted,
                                base::Unretained(this), std::move(callback),
                                contribution_id, fee, info->publisher_key));
}

void Gemini::ContributionCompleted(ledger::LegacyResultCallback callback,
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

void Gemini::FetchBalance(FetchBalanceCallback callback) {
  auto wallet = GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::LEDGER_OK, 0.0);
  }

  auto url_callback = base::BindOnce(
      &Gemini::OnFetchBalance, base::Unretained(this), std::move(callback));

  gemini_server_->post_balance()->Request(wallet->token,
                                          std::move(url_callback));
}

void Gemini::OnFetchBalance(FetchBalanceCallback callback,
                            const mojom::Result result,
                            const double available) {
  if (!GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR, 0.0);
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    if (!LogOutWallet()) {
      BLOG(0, "Failed to disconnect " << constant::kWalletGemini << " wallet!");
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

void Gemini::TransferFunds(double amount,
                           const std::string& address,
                           const std::string& contribution_id,
                           client::LegacyResultCallback callback) {
  transfer_->Run(contribution_id, address, amount,
                 base::BindOnce([](client::LegacyResultCallback callback,
                                   mojom::Result result) { callback(result); },
                                std::move(callback)));
}

void Gemini::ConnectWallet(const base::flat_map<std::string, std::string>& args,
                           ledger::ConnectExternalWalletCallback callback) {
  connect_wallet_->Run(args, std::move(callback));
}

void Gemini::GetWallet(GetExternalWalletCallback callback) {
  get_wallet_->Run(std::move(callback));
}

void Gemini::SaveTransferFee(const std::string& contribution_id,
                             const double fee) {
  StartTransferFeeTimer(contribution_id, 1);

  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  wallet->fees.insert(std::make_pair(contribution_id, fee));
  if (!SetWallet(std::move(wallet))) {
    BLOG(0, "Failed to set " << constant::kWalletGemini << " wallet!");
  }
}

void Gemini::StartTransferFeeTimer(const std::string& fee_id,
                                   const int attempts) {
  DCHECK(!fee_id.empty());

  base::TimeDelta delay = base::Minutes(5);

  BLOG(1, "Gemini transfer fee timer set for " << delay);

  transfer_fee_timers_[fee_id].Start(
      FROM_HERE, delay,
      base::BindOnce(&Gemini::OnTransferFeeTimerElapsed, base::Unretained(this),
                     fee_id, attempts));
}

void Gemini::OnTransferFeeCompleted(const std::string& contribution_id,
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

void Gemini::TransferFee(const std::string& contribution_id,
                         const double amount,
                         const int attempts) {
  transfer_->Run(
      contribution_id, GetFeeAddress(), amount,
      base::BindOnce(&Gemini::OnTransferFeeCompleted, base::Unretained(this),
                     contribution_id, attempts));
}

void Gemini::OnTransferFeeTimerElapsed(const std::string& id,
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

mojom::ExternalWalletPtr Gemini::GetWallet() {
  return ledger::wallet::GetWallet(ledger_, constant::kWalletGemini);
}

mojom::ExternalWalletPtr Gemini::GetWalletIf(
    const std::set<mojom::WalletStatus>& statuses) {
  return ledger::wallet::GetWalletIf(ledger_, constant::kWalletGemini,
                                     statuses);
}

bool Gemini::SetWallet(mojom::ExternalWalletPtr wallet) {
  return ledger::wallet::SetWallet(ledger_, std::move(wallet));
}

bool Gemini::LogOutWallet() {
  return ledger::wallet::LogOutWallet(ledger_, constant::kWalletGemini);
}

void Gemini::RemoveTransferFee(const std::string& contribution_id) {
  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  wallet->fees.erase(contribution_id);
  if (!SetWallet(std::move(wallet))) {
    BLOG(0, "Failed to set " << constant::kWalletGemini << " wallet!");
  }
}

}  // namespace ledger::gemini
