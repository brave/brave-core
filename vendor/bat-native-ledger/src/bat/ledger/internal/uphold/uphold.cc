/* Copyright (c) 2019 The Brave Authors. All rights reserved.
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
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_authorization.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "bat/ledger/internal/uphold/uphold_transfer.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/uphold/uphold_wallet.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {
  const char kFeeMessage[] =
      "5% transaction fee collected by Brave Software International";
}  // namespace

namespace ledger {
namespace uphold {

Uphold::Uphold(LedgerImpl* ledger) :
    transfer_(std::make_unique<UpholdTransfer>(ledger, this)),
    card_(std::make_unique<UpholdCard>(ledger, this)),
    user_(std::make_unique<UpholdUser>(ledger)),
    authorization_(std::make_unique<UpholdAuthorization>(ledger, this)),
    wallet_(std::make_unique<UpholdWallet>(ledger, this)),
    uphold_server_(std::make_unique<ledger::endpoint::UpholdServer>(ledger)),
    ledger_(ledger) {
}

Uphold::~Uphold() = default;

void Uphold::Initialize() {
  auto fees = ledger_->ledger_client()->GetTransferFees(
      constant::kWalletUphold);
  for (auto const& value : fees) {
    if (value.second) {
      StartTransferFeeTimer(value.second->id);
    }
  }
}

void Uphold::StartContribution(
    const std::string& contribution_id,
    type::ServerPublisherInfoPtr info,
    const double amount,
    ledger::ResultCallback callback) {
  if (!info) {
    BLOG(0, "Publisher info is null");
    ContributionCompleted(
        type::Result::LEDGER_ERROR,
        "",
        contribution_id,
        amount,
        "",
        callback);
    return;
  }

  const double fee = (amount * 1.05) - amount;
  const double reconcile_amount = amount - fee;

  auto contribution_callback = std::bind(&Uphold::ContributionCompleted,
      this,
      _1,
      _2,
      contribution_id,
      fee,
      info->publisher_key,
      callback);

  Transaction transaction;
  transaction.address = info->address;
  transaction.amount = reconcile_amount;

  transfer_->Start(transaction, contribution_callback);
}

void Uphold::ContributionCompleted(
    const type::Result result,
    const std::string& transaction_id,
    const std::string& contribution_id,
    const double fee,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  if (result == type::Result::LEDGER_OK) {
    auto transfer_fee = type::TransferFee::New();
    transfer_fee->id = contribution_id;
    transfer_fee->amount = fee;
    SaveTransferFee(std::move(transfer_fee));

    if (!publisher_key.empty()) {
      ledger_->database()->UpdateContributionInfoContributedAmount(
          contribution_id,
          publisher_key,
          callback);
      return;
    }
  }

  callback(result);
}

void Uphold::FetchBalance(FetchBalanceCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  const auto wallet = GetWallet(std::move(wallets));

  if (!wallet ||
      wallet->token.empty() ||
      wallet->address.empty()) {
    BLOG(1, "Wallet data is empty");
    callback(type::Result::LEDGER_OK, 0.0);
    return;
  }

  if (wallet->status == type::WalletStatus::CONNECTED) {
    BLOG(1, "Wallet is connected");
    callback(type::Result::LEDGER_OK, 0.0);
    return;
  }

  auto url_callback = std::bind(&Uphold::OnFetchBalance,
      this,
      _1,
      _2,
      callback);

  uphold_server_->get_card()->Request(
      wallet->address,
      wallet->token,
      url_callback);
}

void Uphold::OnFetchBalance(
    const type::Result result,
    const double available,
    FetchBalanceCallback callback) {
  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    DisconnectWallet();
    callback(type::Result::EXPIRED_TOKEN, 0.0);
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get balance");
    callback(type::Result::LEDGER_ERROR, 0.0);
    return;
  }

  callback(type::Result::LEDGER_OK, available);
}

void Uphold::TransferFunds(
    const double amount,
    const std::string& address,
    client::TransactionCallback callback) {
  Transaction transaction;
  transaction.address = address;
  transaction.amount = amount;
  transfer_->Start(transaction, callback);
}

void Uphold::WalletAuthorization(
    const std::map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  authorization_->Authorize(args, callback);
}

void Uphold::GenerateExternalWallet(ledger::ResultCallback callback) {
  wallet_->Generate(callback);
}

void Uphold::CreateCard(CreateCardCallback callback) {
  card_->CreateIfNecessary(callback);
}

void Uphold::DisconnectWallet() {
  BLOG(1, "Disconnecting wallet");
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));

  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  if (!wallet->address.empty()) {
    ledger_->database()->SaveEventLog(
        ledger::log::kWalletDisconnected,
        static_cast<std::string>(constant::kWalletUphold) + "/" +
            wallet->address.substr(0, 5));
  }

  wallet = wallet::ResetWallet(std::move(wallet));

  ledger_->ledger_client()->ShowNotification(
      "wallet_disconnected",
      {},
      [](type::Result _){});

  ledger_->ledger_client()->SaveExternalWallet(
      constant::kWalletUphold,
      std::move(wallet));
  ledger_->ledger_client()->WalletDisconnected(constant::kWalletUphold);
}

void Uphold::GetUser(GetUserCallback callback) {
  user_->Get(callback);
}

void Uphold::SaveTransferFee(type::TransferFeePtr transfer_fee) {
  if (!transfer_fee) {
    BLOG(0, "Transfer fee is null");
    return;
  }

  StartTransferFeeTimer(transfer_fee->id);
  ledger_->ledger_client()->SetTransferFee(
      constant::kWalletUphold,
      std::move(transfer_fee));
}

void Uphold::StartTransferFeeTimer(const std::string& fee_id) {
  DCHECK(!fee_id.empty());

  base::TimeDelta delay = util::GetRandomizedDelay(
      base::TimeDelta::FromSeconds(45));

  BLOG(1, "Uphold transfer fee timer set for " << delay);

  transfer_fee_timers_[fee_id].Start(FROM_HERE, delay,
      base::BindOnce(&Uphold::OnTransferFeeTimerElapsed,
          base::Unretained(this),
          fee_id));
}

void Uphold::OnTransferFeeCompleted(
    const type::Result result,
    const std::string& transaction_id,
    const type::TransferFee& transfer_fee) {
  if (result == type::Result::LEDGER_OK) {
    ledger_->ledger_client()->RemoveTransferFee(
        constant::kWalletUphold,
        transfer_fee.id);
    return;
  }

  SaveTransferFee(type::TransferFee::New(transfer_fee));
}

void Uphold::TransferFee(const type::TransferFee& transfer_fee) {
  auto transfer_callback = std::bind(&Uphold::OnTransferFeeCompleted,
      this,
      _1,
      _2,
      transfer_fee);

  Transaction transaction;
  transaction.address = GetFeeAddress();
  transaction.amount = transfer_fee.amount;
  transaction.message = kFeeMessage;

  transfer_->Start(transaction, transfer_callback);
}

void Uphold::OnTransferFeeTimerElapsed(const std::string& id) {
  transfer_fee_timers_.erase(id);

  const auto fees = ledger_->ledger_client()->GetTransferFees(
      constant::kWalletUphold);

  for (auto const& value : fees) {
    const auto fee = *value.second;
    if (fee.id == id) {
      TransferFee(fee);
      return;
    }
  }
}

}  // namespace uphold
}  // namespace ledger
