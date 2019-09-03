/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_authorization.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/uphold/uphold_transfer.h"
#include "bat/ledger/internal/uphold/uphold_wallet.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

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

void Uphold::StartContribution(const std::string& viewing_id,
                               ledger::ExternalWalletPtr wallet) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);

  for (const auto& item : reconcile.directions_) {
    auto callback = std::bind(&Uphold::OnServerPublisherInfo,
        this,
        _1,
        viewing_id,
        item.amount_,
        *wallet);

    ledger_->GetServerPublisherInfo(item.publisher_key_, callback);
  }
}

void Uphold::OnServerPublisherInfo(
    ledger::ServerPublisherInfoPtr info,
    const std::string& viewing_id,
    int amount,
    const ledger::ExternalWallet& wallet) {
  if (!info || info->address.empty()) {
    ContributionCompleted(ledger::Result::LEDGER_ERROR, false, viewing_id);
    return;
  }

  const double amount_double = static_cast<double>(amount);
  const double fee = (amount_double * 1.05) - amount_double;
  const double reconcile_amount = amount_double - fee;

  // rest of the reconcile
  auto contribution_callback = std::bind(&Uphold::ContributionCompleted,
                            this,
                            _1,
                            _2,
                            viewing_id,
                            fee,
                            wallet);

  transfer_->Start(reconcile_amount,
                   info->address,
                   ledger::ExternalWallet::New(wallet),
                   contribution_callback);
}

void Uphold::ContributionCompleted(
    const ledger::Result result,
    const bool created,
    const std::string& viewing_id,
    const double fee,
    const ledger::ExternalWallet& wallet) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);
  const auto amount = ConvertToProbi(std::to_string(reconcile.fee_));

  if (result == ledger::Result::LEDGER_OK) {
    // 5% fee
    auto fee_callback = std::bind(&Uphold::OnFeeCompleted,
                              this,
                              _1,
                              _2,
                              viewing_id);

    transfer_->Start(fee,
                     GetFeeAddress(),
                     ledger::ExternalWallet::New(wallet),
                     fee_callback);
  }

  ledger_->OnReconcileComplete(result,
                               viewing_id,
                               amount,
                               reconcile.category_);

  if (result != ledger::Result::LEDGER_OK) {
    if (!viewing_id.empty()) {
      ledger_->RemoveReconcileById(viewing_id);
    }
    return;
  }
}

void Uphold::OnFeeCompleted(ledger::Result result,
                          bool created,
                          const std::string &viewing_id) {
}

void Uphold::FetchBalance(
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    FetchBalanceCallback callback) {
  const auto wallet = GetWallet(std::move(wallets));

  if (!wallet || wallet->token.empty() || wallet->address.empty()) {
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
      ledger::URL_METHOD::GET,
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

  auto* available = dictionary->FindKey("available");
  if (available && available->is_string()) {
    callback(ledger::Result::LEDGER_OK, std::stod(available->GetString()));
    return;
  }

  callback(ledger::Result::LEDGER_ERROR, 0.0);
}

void Uphold::TransferFunds(double amount,
                           const std::string& address,
                           ledger::ExternalWalletPtr wallet,
                           TransactionCallback callback) {
  transfer_->Start(amount, address, std::move(wallet), callback);
}

void Uphold::WalletAuthorization(
    const std::map<std::string, std::string>& args,
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletAuthorizationCallback callback) {
  authorization_->Authorize(args, std::move(wallets), callback);
}

void Uphold::TransferAnonToExternalWallet(
    ledger::ExternalWalletPtr wallet,
    const bool allow_zero_balance,
    ledger::ExternalWalletCallback callback) {
  auto transfer_callback = std::bind(
    &Uphold::OnTransferAnonToExternalWalletCallback,
    this,
    callback,
    *wallet,
    _1);

  // transfer funds from anon wallet to uphold
  ledger_->TransferAnonToExternalWallet(
    std::move(wallet),
    allow_zero_balance,
    transfer_callback);
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
  if (result == ledger::Result::LEDGER_OK) {
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

}  // namespace braveledger_uphold
