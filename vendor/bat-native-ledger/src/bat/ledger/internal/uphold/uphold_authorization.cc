/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/response/response_uphold.h"
#include "bat/ledger/internal/uphold/uphold_authorization.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace braveledger_uphold {

UpholdAuthorization::UpholdAuthorization(
    bat_ledger::LedgerImpl* ledger, Uphold* uphold) :
    ledger_(ledger),
    uphold_(uphold) {
}

UpholdAuthorization::~UpholdAuthorization() = default;

void UpholdAuthorization::Authorize(
    const std::map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet = GetWallet(std::move(wallets));

  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }
  const auto current_one_time = wallet->one_time_string;

  // we need to generate new string as soon as authorization is triggered
  wallet->one_time_string = GenerateRandomString(ledger::is_testing);
  ledger_->ledger_client()->SaveExternalWallet(
      ledger::kWalletUphold,
      wallet->Clone());

  auto it = args.find("error_description");
  if (it != args.end()) {
    const std::string message = args.at("error_description");
    BLOG(1, message);
    if (message == "User does not meet minimum requirements") {
      callback(ledger::Result::NOT_FOUND, {});
      return;
    }

    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  if (args.empty()) {
    BLOG(0, "Arguments are empty");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  std::string code;
  it = args.find("code");
  if (it != args.end()) {
    code = args.at("code");
  }

  if (code.empty()) {
    BLOG(0, "Code is empty");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  std::string one_time_string;
  it = args.find("state");
  if (it != args.end()) {
    one_time_string = args.at("state");
  }

  if (one_time_string.empty()) {
    BLOG(0, "One time string is empty");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  if (current_one_time != one_time_string) {
    BLOG(0, "One time string miss match");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  auto headers = RequestAuthorization();
  const std::string payload =
      base::StringPrintf("code=%s&grant_type=authorization_code", code.c_str());
  const std::string url = GetAPIUrl("/oauth2/token");
  auto auth_callback = std::bind(&UpholdAuthorization::OnAuthorize,
      this,
      _1,
      callback);

  ledger_->LoadURL(
      url,
      headers,
      payload,
      "application/x-www-form-urlencoded",
      ledger::UrlMethod::POST,
      auth_callback);
}

void UpholdAuthorization::OnAuthorize(
    const ledger::UrlResponse& response,
    ledger::ExternalWalletAuthorizationCallback callback) {
  BLOG(7, ledger::UrlResponseToString(__func__, response));

  std::string token;
  const ledger::Result result =
      braveledger_response_util::ParseUpholdAuthorization(response, &token);

  if (result == ledger::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(ledger::Result::EXPIRED_TOKEN, {});
    uphold_->DisconnectWallet();
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get token");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  if (token.empty()) {
    BLOG(0, "Token is empty");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet_ptr = GetWallet(std::move(wallets));

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

  ledger_->ledger_client()->SaveExternalWallet(
      ledger::kWalletUphold,
      wallet_ptr->Clone());

  auto user_callback = std::bind(&UpholdAuthorization::OnGetUser,
      this,
      _1,
      _2,
      callback);
  uphold_->GetUser(user_callback);
}

void UpholdAuthorization::OnGetUser(
    const ledger::Result result,
    const User& user,
    ledger::ExternalWalletAuthorizationCallback callback) {
  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet_ptr = GetWallet(std::move(wallets));
  std::map<std::string, std::string> args;

  if (user.bat_not_allowed || !wallet_ptr) {
    BLOG(0, "BAT not allowed");
    callback(ledger::Result::BAT_NOT_ALLOWED, args);
    return;
  }

  if (user.status == UserStatus::OK) {
    wallet_ptr->status = user.verified
        ? ledger::WalletStatus::VERIFIED
        : ledger::WalletStatus::CONNECTED;
    ledger_->ledger_client()->SaveExternalWallet(
        ledger::kWalletUphold,
        wallet_ptr->Clone());

    if (wallet_ptr->address.empty()) {
      auto new_callback = std::bind(&UpholdAuthorization::OnCardCreate,
          this,
          _1,
          _2,
          callback);
      uphold_->CreateCard(new_callback);
      return;
    }

    if (!user.verified) {
      args["redirect_url"] = GetSecondStepVerify();
    }
  } else {
    wallet_ptr->status = ledger::WalletStatus::PENDING;
    ledger_->ledger_client()->SaveExternalWallet(
        ledger::kWalletUphold,
        std::move(wallet_ptr));
    args["redirect_url"] = GetSecondStepVerify();
  }

  callback(ledger::Result::LEDGER_OK, args);
}

void UpholdAuthorization::OnCardCreate(
    const ledger::Result result,
    const std::string& address,
    ledger::ExternalWalletAuthorizationCallback callback) {
  if (result == ledger::Result::LEDGER_ERROR) {
    BLOG(0, "Card creation");
    callback(ledger::Result::LEDGER_ERROR, {});
    return;
  }

  auto wallets = ledger_->ledger_client()->GetExternalWallets();
  auto wallet_ptr = GetWallet(std::move(wallets));
  wallet_ptr->address = address;
  ledger_->ledger_client()->SaveExternalWallet(
      ledger::kWalletUphold,
      wallet_ptr->Clone());

  if (!address.empty()) {
    ledger_->database()->SaveEventLog(
        ledger::log::kWalletConnected,
        static_cast<std::string>(ledger::kWalletUphold) + "/" +
            address.substr(0, 5));
  }

  std::map<std::string, std::string> args;
  if (wallet_ptr->status != ledger::WalletStatus::VERIFIED) {
    args["redirect_url"] = GetSecondStepVerify();
  }

  callback(ledger::Result::LEDGER_OK, args);
}

}  // namespace braveledger_uphold
