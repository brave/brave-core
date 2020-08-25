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
#include "bat/ledger/internal/bat_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/response/response_uphold.h"
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

namespace braveledger_uphold {

Uphold::Uphold(bat_ledger::LedgerImpl* ledger) :
    transfer_(std::make_unique<UpholdTransfer>(ledger, this)),
    card_(std::make_unique<UpholdCard>(ledger, this)),
    user_(std::make_unique<UpholdUser>(ledger)),
    authorization_(std::make_unique<UpholdAuthorization>(ledger, this)),
    wallet_(std::make_unique<UpholdWallet>(ledger, this)),
    ledger_(ledger) {
}

Uphold::~Uphold() = default;

void Uphold::Initialize() {
  auto fees = ledger_->GetTransferFees(ledger::kWalletUphold);
  for (auto const& value : fees) {
    if (!value.second) {
      continue;
    }

    SaveTransferFee(value.second->Clone());
  }
}

void Uphold::StartContribution(
    const std::string& contribution_id,
    ledger::ServerPublisherInfoPtr info,
    const double amount,
    ledger::ResultCallback callback) {
  if (!info) {
    BLOG(0, "Publisher info is null");
    ContributionCompleted(
        ledger::Result::LEDGER_ERROR,
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
    const ledger::Result result,
    const std::string& transaction_id,
    const std::string& contribution_id,
    const double fee,
    const std::string& publisher_key,
    ledger::ResultCallback callback) {
  if (result == ledger::Result::LEDGER_OK) {
    const auto current_time_seconds =
        braveledger_time_util::GetCurrentTimeStamp();
    auto transfer_fee = ledger::TransferFee::New();
    transfer_fee->id = contribution_id;
    transfer_fee->amount = fee;
    transfer_fee->execution_timestamp =
        current_time_seconds + brave_base::random::Geometric(60);
    SaveTransferFee(std::move(transfer_fee));

    if (!publisher_key.empty()) {
      ledger_->UpdateContributionInfoContributedAmount(
        contribution_id,
        publisher_key,
        callback);
      return;
    }
  }

  callback(result);
}

void Uphold::FetchBalance(FetchBalanceCallback callback) {
  auto wallets = ledger_->GetExternalWallets();
  const auto wallet = GetWallet(std::move(wallets));

  if (!wallet ||
      wallet->token.empty() ||
      wallet->address.empty()) {
    BLOG(1, "Wallet data is empty");
    callback(ledger::Result::LEDGER_OK, 0.0);
    return;
  }

  if (wallet->status == ledger::WalletStatus::CONNECTED) {
    BLOG(1, "Wallet is connected");
    callback(ledger::Result::LEDGER_OK, 0.0);
    return;
  }

  auto headers = RequestAuthorization(wallet->token);
  const std::string url = GetAPIUrl("/v0/me/cards/" + wallet->address);

  auto balance_callback = std::bind(&Uphold::OnFetchBalance,
      this,
      _1,
      callback);

  ledger_->LoadURL(
      url,
      headers,
      "",
      "",
      ledger::UrlMethod::GET,
      balance_callback);
}

void Uphold::OnFetchBalance(
    const ledger::UrlResponse& response,
    FetchBalanceCallback callback) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));

  double available = 0.0;
  const ledger::Result result =
      braveledger_response_util::ParseFetchUpholdBalance(response, &available);
  if (result == ledger::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    DisconnectWallet();
    callback(ledger::Result::EXPIRED_TOKEN, available);
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get balance");
    callback(ledger::Result::LEDGER_ERROR, available);
    return;
  }

  callback(ledger::Result::LEDGER_OK, available);
}

void Uphold::TransferFunds(
    const double amount,
    const std::string& address,
    ledger::TransactionCallback callback) {
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
  auto wallets = ledger_->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));

  if (!wallet) {
    BLOG(0, "Wallet is null");
    return;
  }

  wallet = braveledger_wallet::ResetWallet(std::move(wallet));

  ledger_->ShowNotification(
    "wallet_disconnected",
    [](ledger::Result _){});

  ledger_->SaveExternalWallet(ledger::kWalletUphold, std::move(wallet));
}

void Uphold::GetUser(GetUserCallback callback) {
  user_->Get(callback);
}

void Uphold::SaveTransferFee(ledger::TransferFeePtr transfer_fee) {
  if (!transfer_fee) {
    BLOG(0, "Transfer fee is null");
    return;
  }

  auto timer_id = 0u;
  SetTimer(&timer_id);
  transfer_fee->execution_id = timer_id;
  ledger_->SetTransferFee(ledger::kWalletUphold, std::move(transfer_fee));
}

void Uphold::OnTransferFeeCompleted(
    const ledger::Result result,
    const std::string& transaction_id,
    const ledger::TransferFee& transfer_fee) {
  if (result == ledger::Result::LEDGER_OK) {
    ledger_->RemoveTransferFee(ledger::kWalletUphold, transfer_fee.id);
    return;
  }

  SaveTransferFee(ledger::TransferFee::New(transfer_fee));
}

void Uphold::TransferFee(const ledger::TransferFee& transfer_fee) {
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

void Uphold::OnTimer(const uint32_t timer_id) {
  const auto fees = ledger_->GetTransferFees(ledger::kWalletUphold);

  for (auto const& value : fees) {
    const auto fee = *value.second;
    if (fee.execution_id == timer_id) {
      TransferFee(fee);
      return;
    }
  }
}

void Uphold::SetTimer(uint32_t* timer_id, uint64_t start_timer_in) {
  if (start_timer_in == 0) {
    start_timer_in = brave_base::random::Geometric(45);
  }

  BLOG(1, "Starts Uphold timer in " << start_timer_in);

  ledger_->SetTimer(start_timer_in, timer_id);
}

}  // namespace braveledger_uphold
