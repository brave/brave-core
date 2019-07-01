/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/base64.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_split.h"
#include "bat/ledger/internal/uphold/uphold.h"
#include "bat/ledger/internal/uphold/uphold_transfer.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_uphold {

Uphold::Uphold(bat_ledger::LedgerImpl* ledger) :
    transfer_(std::make_unique<UpholdTransfer>(ledger, this)),
    ledger_(ledger) {
}

Uphold::~Uphold() {
}

std::vector<std::string> Uphold::RequestAuthorization(
    const std::string& token) {
  std::vector<std::string> headers;

  if (!token.empty()) {
    headers.push_back("Authorization: Bearer " + token);
    return headers;
  }

  const std::string id = GetClientId();
  const std::string secret = GetClientSecret();

  std::string user;
  base::Base64Encode(base::StringPrintf("%s:%s", id.c_str(), secret.c_str()),
                     &user);

  headers.push_back("Authorization: Basic " + user);

  return headers;
}

ledger::ExternalWalletPtr Uphold::GetWallet(
    std::map<std::string, ledger::ExternalWalletPtr> wallets) {
  for (auto& wallet : wallets) {
    if (wallet.first == ledger::kWalletUphold) {
      return std::move(wallet.second);
    }
  }

  return nullptr;
}

std::string Uphold::GetClientId() {
  return ledger::is_production
      ? kClientIdProduction
      : kClientIdStaging;
}

std::string Uphold::GetClientSecret() {
  return ledger::is_production
      ? kClientSecretProduction
      : kClientSecretStaging;
}

std::string Uphold::GetUrl() {
  return ledger::is_production
      ? kUrlProduction
      : kUrlStaging;
}

std::string Uphold::GetAPIUrl(const std::string& path) {
  std::string url;
  if (ledger::is_production) {
    url = kAPIUrlProduction;
  } else {
    url = kAPIUrlStaging;
  }

  return url + path;
}

std::string Uphold::GetFeeAddress() {
  return ledger::is_production
      ? kFeeAddressProduction
      : kFeeAddressStaging;
}

// TODO add test
std::string Uphold::ConvertToProbi(const std::string& amount) {
  auto vec = base::SplitString(
      amount, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  const std::string probi = "000000000000000000";

  if (vec.size() == 1) {
    return vec.at(0) + probi;
  }

  const auto before_dot = vec.at(0);
  const auto after_dot = vec.at(1);
  const auto rest_probi = probi.substr(after_dot.size());

  return before_dot + after_dot + rest_probi;
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
    auto fee_callback = std::bind(&Uphold::FeeCompleted,
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

void Uphold::FeeCompleted(ledger::Result result,
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
  if (available) {
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

std::string Uphold::GetVerifyUrl(const std::string& state) {
  const std::string id = GetClientId();

  const std::string url = GetUrl();

  return base::StringPrintf(
      "%s/authorize/%s"
      "?scope=cards:read&cards:write&transactions:read&"
      "transactions:transfer:application&"
      "transactions:transfer:others&intention=kyc&"
      "state=%s"
      , url.c_str(), id.c_str(), state.c_str());
}

std::string Uphold::GetAddUrl(const std::string& address) {
  const std::string url = GetUrl();

  return base::StringPrintf(
      "%s/dashboard/cards/%s/add", url.c_str(), address.c_str());
}

std::string Uphold::GetWithdrawUrl(const std::string& address) {
  const std::string url = GetUrl();

  return base::StringPrintf(
      "%s/dashboard/cards/%s/use", url.c_str(), address.c_str());
}

std::string Uphold::GetSecondStepVerify() {
  const std::string url = GetUrl();
  const std::string id = GetClientId();

  return base::StringPrintf(
      "%s/signup/step2?application_id=%s&intention=kyc",
      url.c_str(),
      id.c_str());
}

void Uphold::OnWalletAuthorization(
    ledger::ExternalWalletAuthorizationCallback callback,
    ledger::ExternalWallet wallet,
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
  if (access_token) {
    token = access_token->GetString();
  }

  if (token.empty()) {
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  wallet.token = token;

  switch (wallet.status) {
    case ledger::WalletStatus::NOT_CONNECTED: {
      // TODO create new BAT card
      wallet.address = "c5fd7219-6586-4fe1-b947-0cbd25040ca8";
      wallet.status = ledger::WalletStatus::CONNECTED;
      break;
    }
    case ledger::WalletStatus::DISCONNECTED_NOT_VERIFIED: {
      wallet.status = ledger::WalletStatus::CONNECTED;
      break;
    }
    case ledger::WalletStatus::DISCONNECTED_VERIFIED: {
      wallet.status = ledger::WalletStatus::VERIFIED;
      break;
    }
    default:
      break;
  }

  auto wallet_ptr = ledger::ExternalWallet::New(wallet);

  // TODO we need to fetch user details so that we know if card is KYC
  // if it's KYC just change status to VERIFIED and return empty url
  // if it's not KYC then we should return second step url

  ledger_->SaveExternalWallet(ledger::kWalletUphold, std::move(wallet_ptr));

  std::map<std::string, std::string> args;
  args["redirect_url"] = GetSecondStepVerify();
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

  wallet->one_time_string = GenerateRandomString(30);
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

}  // namespace braveledger_uphold
