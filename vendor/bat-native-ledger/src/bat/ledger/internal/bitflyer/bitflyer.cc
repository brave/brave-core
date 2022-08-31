/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer.h"
#include "bat/ledger/internal/bitflyer/bitflyer_authorization.h"
#include "bat/ledger/internal/bitflyer/bitflyer_transfer.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/bitflyer/bitflyer_wallet.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_server.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/notifications/notification_keys.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {
const char kFeeMessage[] =
    "5% transaction fee collected by Brave Software International";
}  // namespace

namespace ledger {
namespace bitflyer {

Bitflyer::Bitflyer(LedgerImpl* ledger)
    : WalletProvider(ledger),
      transfer_(std::make_unique<BitflyerTransfer>(ledger)),
      authorization_(std::make_unique<BitflyerAuthorization>(ledger)),
      wallet_(std::make_unique<BitflyerWallet>(ledger)),
      bitflyer_server_(std::make_unique<endpoint::BitflyerServer>(ledger)),
      ledger_(ledger) {}

Bitflyer::~Bitflyer() = default;

const char* Bitflyer::Name() const {
  return constant::kWalletBitflyer;
}

type::ExternalWalletPtr Bitflyer::GenerateLinks(type::ExternalWalletPtr wallet) {
  return bitflyer::GenerateLinks(std::move(wallet));
}

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
                                 type::ServerPublisherInfoPtr info,
                                 double amount,
                                 ledger::LegacyResultCallback callback) {
  if (!info) {
    BLOG(0, "Publisher info is null");
    ContributionCompleted(type::Result::LEDGER_ERROR, "", contribution_id,
                          amount, "", callback);
    return;
  }

  const double fee = (amount * 1.05) - amount;
  const double reconcile_amount = amount - fee;

  auto contribution_callback =
      std::bind(&Bitflyer::ContributionCompleted, this, _1, _2, contribution_id,
                fee, info->publisher_key, callback);

  Transaction transaction;
  transaction.address = info->address;
  transaction.amount = reconcile_amount;

  transfer_->Start(transaction, contribution_callback);
}

void Bitflyer::ContributionCompleted(type::Result result,
                                     const std::string& transaction_id,
                                     const std::string& contribution_id,
                                     double fee,
                                     const std::string& publisher_key,
                                     ledger::LegacyResultCallback callback) {
  if (result == type::Result::LEDGER_OK) {
    SaveTransferFee(contribution_id, fee);

    if (!publisher_key.empty()) {
      ledger_->database()->UpdateContributionInfoContributedAmount(
          contribution_id, publisher_key, callback);
      return;
    }
  }

  callback(result);
}

void Bitflyer::FetchBalance(FetchBalanceCallback callback) {
  const auto wallet = GetWallet();

  if (!wallet || wallet->token.empty() || wallet->address.empty()) {
    std::move(callback).Run(type::Result::LEDGER_OK, 0.0);
    return;
  }

  if (wallet->status == type::WalletStatus::CONNECTED) {
    BLOG(1, "Wallet is connected");
    std::move(callback).Run(type::Result::LEDGER_OK, 0.0);
    return;
  }

  auto url_callback = base::BindOnce(
      &Bitflyer::OnFetchBalance, base::Unretained(this), std::move(callback));

  bitflyer_server_->get_balance()->Request(wallet->token,
                                           std::move(url_callback));
}

void Bitflyer::OnFetchBalance(FetchBalanceCallback callback,
                              const type::Result result,
                              const double available) {
  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    DisconnectWallet(ledger::notifications::kWalletDisconnected);
    std::move(callback).Run(type::Result::EXPIRED_TOKEN, 0.0);
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get balance");
    std::move(callback).Run(type::Result::LEDGER_ERROR, 0.0);
    return;
  }

  std::move(callback).Run(type::Result::LEDGER_OK, available);
}

void Bitflyer::TransferFunds(const double amount,
                             const std::string& address,
                             client::TransactionCallback callback) {
  Transaction transaction;
  transaction.address = address;
  transaction.amount = amount;
  transfer_->Start(transaction, callback);
}

void Bitflyer::WalletAuthorization(
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  authorization_->Authorize(args, callback);
}

void Bitflyer::GenerateWallet(ledger::ResultCallback callback) {
  wallet_->Generate(std::move(callback));
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
  SetWallet(std::move(wallet));
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

void Bitflyer::OnTransferFeeCompleted(const type::Result result,
                                      const std::string& transaction_id,
                                      const std::string& contribution_id,
                                      const int attempts) {
  if (result != type::Result::LEDGER_OK) {
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
                           const double amount,
                           const int attempts) {
  auto transfer_callback = std::bind(&Bitflyer::OnTransferFeeCompleted, this,
                                     _1, _2, contribution_id, attempts);

  Transaction transaction;
  transaction.address = GetFeeAddress();
  transaction.amount = amount;
  transaction.message = kFeeMessage;

  transfer_->Start(transaction, transfer_callback);
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

void Bitflyer::RemoveTransferFee(const std::string& contribution_id) {
  auto wallet = GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  wallet->fees.erase(contribution_id);
  SetWallet(std::move(wallet));
}

}  // namespace bitflyer
}  // namespace ledger
