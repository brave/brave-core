/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_card.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/uphold/uphold_transfer.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_uphold {

Uphold::Uphold(bat_ledger::LedgerImpl* ledger) :
    transfer_(std::make_unique<UpholdTransfer>(ledger)),
    card_(std::make_unique<UpholdCard>(ledger)),
    ledger_(ledger) {
}

Uphold::~Uphold() {
}

void Uphold::StartContribution(const std::string &viewing_id,
                               ledger::ExternalWalletPtr wallet) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);

  for (const auto& item : reconcile.directions_) {
    const std::string address =
        ledger_->GetPublisherAddress(item.publisher_key_);
    if (address.empty()) {
      ContributionCompleted(ledger::Result::LEDGER_ERROR, false, viewing_id);
      return;
    }

    const double amount = static_cast<double>(item.amount_);
    const double fee = (amount * 1.05) - amount;
    const double reconcile_amount = amount - fee;

    // 5% fee
    auto fee_callback = std::bind(&Uphold::OnFeeCompleted,
                              this,
                              _1,
                              _2,
                              viewing_id);

    transfer_->Start(fee,
                     GetFeeAddress(),
                     wallet->Clone(),
                     fee_callback);

    // rest of the reconcile
    auto contribution_callback = std::bind(&Uphold::ContributionCompleted,
                              this,
                              _1,
                              _2,
                              viewing_id);

    transfer_->Start(reconcile_amount,
                     address,
                     std::move(wallet),
                     contribution_callback);
  }
}

void Uphold::ContributionCompleted(ledger::Result result,
                                   bool created,
                                   const std::string &viewing_id) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);
  const auto amount = ConvertToProbi(std::to_string(reconcile.fee_));

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

  if (!wallet || wallet->token.empty()) {
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
  if (response_status_code == net::HTTP_UNAUTHORIZED ||
      response_status_code == net::HTTP_NOT_FOUND) {
    // TODO add token expiration flow
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

void Uphold::OnWalletAuthorizationCreate(
    ledger::Result result,
    const std::string& address,
    ledger::ExternalWalletAuthorizationCallback callback,
    const ledger::ExternalWallet& wallet) {
  if (result == ledger::Result::LEDGER_ERROR) {
    // TODO(nejczdovc) what to do in this case
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  auto wallet_ptr = ledger::ExternalWallet::New(wallet);

  wallet_ptr->address = address;

  ledger_->SaveExternalWallet(ledger::kWalletUphold, wallet_ptr->Clone());

  std::map<std::string, std::string> args;
  if (wallet_ptr->status != ledger::WalletStatus::VERIFIED) {
    args["redirect_url"] = GetSecondStepVerify();
  }
  callback(ledger::Result::LEDGER_OK, args);
}

void Uphold::OnWalletAuthorization(
    ledger::ExternalWalletAuthorizationCallback callback,
    const ledger::ExternalWallet& wallet,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  std::string token;
  auto* access_token = dictionary->FindKey("access_token");
  if (access_token && access_token->is_string()) {
    token = access_token->GetString();
  }

  if (token.empty()) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  auto wallet_ptr = ledger::ExternalWallet::New(wallet);

  wallet_ptr->token = token;

  switch (wallet_ptr->status) {
    case ledger::WalletStatus::NOT_CONNECTED: {
      wallet_ptr->status = ledger::WalletStatus::CONNECTED;
      break;
    }
    case ledger::WalletStatus::DISCONNECTED_NOT_VERIFIED: {
      wallet_ptr->status = ledger::WalletStatus::CONNECTED;
      break;
    }
    case ledger::WalletStatus::DISCONNECTED_VERIFIED: {
      wallet_ptr->status = ledger::WalletStatus::VERIFIED;
      break;
    }
    default:
      break;
  }

  if (wallet_ptr->address.empty()) {
    auto new_callback = std::bind(&Uphold::OnWalletAuthorizationCreate,
                                    this,
                                    _1,
                                    _2,
                                    callback,
                                    *wallet_ptr);
    CreateCard(std::move(wallet_ptr), new_callback);
    return;
  }

  ledger_->SaveExternalWallet(ledger::kWalletUphold, wallet_ptr->Clone());

  std::map<std::string, std::string> args;
  if (wallet_ptr->status != ledger::WalletStatus::VERIFIED) {
    args["redirect_url"] = GetSecondStepVerify();
  }

  callback(ledger::Result::LEDGER_OK, args);
}

void Uphold::WalletAuthorization(
    const std::map<std::string, std::string>& args,
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletAuthorizationCallback callback) {
  auto wallet = GetWallet(std::move(wallets));

  if (!wallet) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  const auto current_one_time = wallet->one_time_string;

  wallet->one_time_string = GenerateRandomString();
  ledger_->SaveExternalWallet(ledger::kWalletUphold, wallet->Clone());

  if (args.empty()) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  std::string code;
  auto it = args.find("code");
  if (it != args.end()) {
    code = args.at("code");
  }

  if (code.empty()) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  std::string one_time_string;
  it = args.find("state");
  if (it != args.end()) {
    one_time_string = args.at("state");
  }

  if (one_time_string.empty()) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  if (current_one_time != one_time_string) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  auto headers = RequestAuthorization();
  const std::string payload =
      base::StringPrintf("code=%s&grant_type=authorization_code", code.c_str());
  const std::string url = GetAPIUrl("/oauth2/token");
  auto auth_callback = std::bind(&Uphold::OnWalletAuthorization,
                                    this,
                                    callback,
                                    *wallet,
                                    _1,
                                    _2,
                                    _3);

  ledger_->LoadURL(
      url,
      headers,
      payload,
      "application/x-www-form-urlencoded",
      ledger::URL_METHOD::POST,
      auth_callback);
}

void Uphold::OnGenerateExternalWallet(
    ledger::Result result,
    ledger::ExternalWalletPtr wallet,
    ledger::ExternalWalletCallback callback) {
  ledger_->SaveExternalWallet(ledger::kWalletUphold, wallet->Clone());
  callback(std::move(wallet));
}

void Uphold::GenerateExternalWallet(
    std::map<std::string, ledger::ExternalWalletPtr> wallets,
    ledger::ExternalWalletCallback callback) {
  ledger::ExternalWalletPtr wallet;
  if (wallets.size() == 0) {
    wallet = ledger::ExternalWallet::New();
    wallet->status = ledger::WalletStatus::NOT_CONNECTED;
  } else {
    wallet = GetWallet(std::move(wallets));

    if (!wallet) {
      wallet = ledger::ExternalWallet::New();
      wallet->status = ledger::WalletStatus::NOT_CONNECTED;
    } else {
      wallet->add_url = GetAddUrl(wallet->address);
      wallet->withdraw_url = GetWithdrawUrl(wallet->address);
    }
  }

  if (wallet->one_time_string.empty()) {
    wallet->one_time_string = GenerateRandomString();
  }

  if (wallet->status == ledger::WalletStatus::CONNECTED) {
    wallet->verify_url = GetSecondStepVerify();
  } else {
    wallet->verify_url = GetVerifyUrl(wallet->one_time_string);
  }

  wallet->account_url = GetAccountUrl();

  if (wallet->status == ledger::WalletStatus::CONNECTED ||
      wallet->status == ledger::WalletStatus::VERIFIED) {
    auto user_callback = std::bind(&Uphold::OnGenerateExternalWallet,
                                   this,
                                   _1,
                                   _2,
                                   callback);
    GetUser(std::move(wallet), user_callback);
    return;
  }

  ledger_->SaveExternalWallet(ledger::kWalletUphold, wallet->Clone());
  callback(std::move(wallet));
}

void Uphold::CreateCard(
    ledger::ExternalWalletPtr wallet,
    CreateCardCallback callback) {
  card_->Create(std::move(wallet), callback);
}

void Uphold::OnGetUser(
    GetUserCallback callback,
    const ledger::ExternalWallet& wallet,
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  ledger_->LogResponse(__func__, response_status_code, response, headers);
  auto new_wallet = ledger::ExternalWallet::New(wallet);
  if (response_status_code != net::HTTP_OK) {
    callback(ledger::Result::LEDGER_ERROR, std::move(new_wallet));
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    callback(ledger::Result::LEDGER_ERROR, std::move(new_wallet));
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    callback(ledger::Result::LEDGER_ERROR, std::move(new_wallet));
    return;
  }

  auto* name = dictionary->FindKey("name");
  if (name && name->is_string()) {
    new_wallet->user_name = name->GetString();
  }

  bool verified = false;
  auto* member_at = dictionary->FindKey("memberAt");
  if (member_at && member_at->is_string()) {
    const auto member = member_at->GetString();
    verified = !member.empty();
  }

  if (new_wallet->status == ledger::WalletStatus::CONNECTED && verified) {
    new_wallet->status = ledger::WalletStatus::VERIFIED;
  } else if (
      new_wallet->status == ledger::WalletStatus::VERIFIED && !verified) {
    new_wallet->status = ledger::WalletStatus::CONNECTED;
  }

  callback(ledger::Result::LEDGER_OK, std::move(new_wallet));
}

void Uphold::GetUser(
    ledger::ExternalWalletPtr wallet,
    GetUserCallback callback) {
  const auto headers = RequestAuthorization(wallet->token);
  const std::string url = GetAPIUrl("/v0/me");

  auto user_callback = std::bind(&Uphold::OnGetUser,
                                 this,
                                 callback,
                                 *wallet,
                                 _1,
                                 _2,
                                 _3);
  ledger_->LoadURL(
      url,
      headers,
      "",
      "",
      ledger::URL_METHOD::GET,
      user_callback);
}

}  // namespace braveledger_uphold
