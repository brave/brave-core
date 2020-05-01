/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bat_util.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_authorization.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/uphold/uphold_transfer.h"
#include "bat/ledger/internal/uphold/uphold_wallet.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"

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

Uphold::~Uphold() {
}

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
    double amount,
    ledger::ExternalWalletPtr wallet,
    ledger::ResultCallback callback) {
  if (!info) {
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

  transfer_->Start(transaction, std::move(wallet), contribution_callback);
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

void Uphold::FetchBalance(
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    FetchBalanceCallback callback) {
  const auto wallet = GetWallet(std::move(wallets));

  if (!wallet ||
      wallet->token.empty() ||
      wallet->address.empty() ||
      wallet->status == ledger::WalletStatus::CONNECTED) {
    callback(ledger::Result::LEDGER_OK, 0.0);
    return;
  }

  auto headers = RequestAuthorization(wallet->token);
  const std::string url = GetAPIUrl("/v0/me/cards/" + wallet->address);

  auto balance_callback = std::bind(&Uphold::OnFetchBalance,
                                    this,
                                    callback,
                                    _1,
                                    _2,
                                    _3);

  ledger_->LoadURL(
      url,
      headers,
      "",
      "",
      ledger::UrlMethod::GET,
      balance_callback);
}

void Uphold::OnFetchBalance(
    FetchBalanceCallback callback,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);

  if (response_status_code == net::HTTP_UNAUTHORIZED) {
    callback(ledger::Result::EXPIRED_TOKEN, 0.0);
    DisconnectWallet();
    return;
  }

  if (response_status_code == net::HTTP_NOT_FOUND) {
    callback(ledger::Result::LEDGER_ERROR, 0.0);
    return;
  }

  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, 0.0);
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    callback(ledger::Result::LEDGER_ERROR, 0.0);
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    callback(ledger::Result::LEDGER_ERROR, 0.0);
    return;
  }

  const auto* available = dictionary->FindStringKey("available");
  if (available) {
    callback(ledger::Result::LEDGER_OK, std::stod(*available));
    return;
  }

  callback(ledger::Result::LEDGER_ERROR, 0.0);
}

void Uphold::TransferFunds(
    const double amount,
    const std::string& address,
    ledger::ExternalWalletPtr wallet,
    ledger::TransactionCallback callback) {
  Transaction transaction;
  transaction.address = address;
  transaction.amount = amount;
  transfer_->Start(transaction, std::move(wallet), callback);
}

void Uphold::WalletAuthorization(
    const std::map<std::string, std::string>& args,
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletAuthorizationCallback callback) {
  authorization_->Authorize(args, std::move(wallets), callback);
}

void Uphold::TransferAnonToExternalWallet(
    ledger::ExternalWalletPtr wallet,
    ledger::ExternalWalletCallback callback) {
  auto transfer_callback = std::bind(
    &Uphold::OnTransferAnonToExternalWalletCallback,
    this,
    callback,
    *wallet,
    _1);

  // transfer funds from anon wallet to uphold
  ledger_->TransferAnonToExternalWallet(std::move(wallet), transfer_callback);
}

void Uphold::GenerateExternalWallet(
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletCallback callback) {
  wallet_->Generate(std::move(wallets), callback);
}

void Uphold::CreateCard(
    ledger::ExternalWalletPtr wallet,
    CreateCardCallback callback) {
  card_->CreateIfNecessary(std::move(wallet), callback);
}

void Uphold::OnTransferAnonToExternalWalletCallback(
    ledger::ExternalWalletCallback callback,
    const ledger::ExternalWallet& wallet,
    ledger::Result result) {
  auto wallet_ptr = ledger::ExternalWallet::New(wallet);
  if (result == ledger::Result::LEDGER_OK ||
      result == ledger::Result::ALREADY_EXISTS) {
    wallet_ptr->transferred = true;
  }

  ledger_->SaveExternalWallet(ledger::kWalletUphold, wallet_ptr->Clone());
  callback(ledger::Result::LEDGER_OK, std::move(wallet_ptr));
}

void Uphold::OnDisconectWallet(
    ledger::Result,
    ledger::ExternalWalletPtr wallet) {
  if (!wallet) {
    return;
  }

  if (wallet->status == ledger::WalletStatus::VERIFIED) {
    wallet->status = ledger::WalletStatus::DISCONNECTED_VERIFIED;
  } else if (wallet->status == ledger::WalletStatus::CONNECTED ||
            wallet->status == ledger::WalletStatus::NOT_CONNECTED) {
    wallet->status = ledger::WalletStatus::DISCONNECTED_NOT_VERIFIED;
  }

  wallet->token = "";

  ledger_->ShowNotification(
    "wallet_disconnected",
    [](ledger::Result _){});

  ledger_->SaveExternalWallet(ledger::kWalletUphold, std::move(wallet));
}

void Uphold::DisconnectWallet() {
  auto callback = std::bind(&Uphold::OnDisconectWallet,
                                 this,
                                 _1,
                                 _2);

  ledger_->GetExternalWallet(ledger::kWalletUphold, callback);
}

void Uphold::GetUser(
    ledger::ExternalWalletPtr wallet,
    GetUserCallback callback) {
  user_->Get(std::move(wallet), callback);
}

void Uphold::CreateAnonAddressIfNecessary(
    ledger::ExternalWalletPtr wallet,
    CreateAnonAddressCallback callback) {
  card_->CreateAnonAddressIfNecessary(std::move(wallet), callback);
}

void Uphold::SaveTransferFee(ledger::TransferFeePtr transfer_fee) {
  if (!transfer_fee) {
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

void Uphold::TransferFee(
    const ledger::Result result,
    ledger::ExternalWalletPtr wallet,
    const ledger::TransferFee& transfer_fee) {

  if (result != ledger::Result::LEDGER_OK) {
    SaveTransferFee(ledger::TransferFee::New(transfer_fee));
    return;
  }

  auto callback = std::bind(&Uphold::OnTransferFeeCompleted,
          this,
          _1,
          _2,
          transfer_fee);

  Transaction transaction;
  transaction.address = GetFeeAddress();
  transaction.amount = transfer_fee.amount;
  transaction.message = kFeeMessage;

  transfer_->Start(transaction, std::move(wallet), callback);
}

void Uphold::TransferFeeOnTimer(const uint32_t timer_id) {
  const auto fees = ledger_->GetTransferFees(ledger::kWalletUphold);

  for (auto const& value : fees) {
    const auto fee = *value.second;
    if (fee.execution_id == timer_id) {
      auto callback = std::bind(&Uphold::TransferFee,
          this,
          _1,
          _2,
          fee);

      ledger_->GetExternalWallet(ledger::kWalletUphold, callback);
      return;
    }
  }
}

void Uphold::OnTimer(uint32_t timer_id) {
  BLOG(ledger_, ledger::LogLevel::LOG_INFO)
    << "OnTimer Uphold: "
    << timer_id;

  TransferFeeOnTimer(timer_id);
}

void Uphold::SetTimer(uint32_t* timer_id, uint64_t start_timer_in) {
  if (start_timer_in == 0) {
    start_timer_in = brave_base::random::Geometric(45);
  }

  BLOG(ledger_, ledger::LogLevel::LOG_INFO)
    << "Starts Uphold timer in "
    << start_timer_in;

  ledger_->SetTimer(start_timer_in, timer_id);
}

}  // namespace braveledger_uphold
