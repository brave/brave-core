/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bitflyer/bitflyer_authorization.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/notifications/notification_keys.h"
#include "crypto/sha2.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

namespace ledger {
namespace bitflyer {

BitflyerAuthorization::BitflyerAuthorization(LedgerImpl* ledger)
    : ledger_(ledger),
      bitflyer_server_(std::make_unique<endpoint::BitflyerServer>(ledger)),
      promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {}

BitflyerAuthorization::~BitflyerAuthorization() = default;

void BitflyerAuthorization::Authorize(
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  auto bitflyer_wallet = ledger_->bitflyer()->GetWallet();
  if (!bitflyer_wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  const auto current_one_time = bitflyer_wallet->one_time_string;
  const auto current_code_verifier = bitflyer_wallet->code_verifier;

  // We need to generate new strings as soon as authorization is triggered
  bitflyer_wallet->one_time_string = util::GenerateRandomHexString();
  bitflyer_wallet->code_verifier = util::GeneratePKCECodeVerifier();
  const bool success = ledger_->bitflyer()->SetWallet(bitflyer_wallet->Clone());

  if (!success) {
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  auto it = args.find("error_description");
  if (it != args.end()) {
    const std::string message = args.at("error_description");
    BLOG(1, message);
    if (message == "User does not meet minimum requirements") {
      callback(type::Result::NOT_FOUND, {});
      return;
    }

    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (args.empty()) {
    BLOG(0, "Arguments are empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  std::string code;
  it = args.find("code");
  if (it != args.end()) {
    code = args.at("code");
  }

  if (code.empty()) {
    BLOG(0, "Code is empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  std::string one_time_string;
  it = args.find("state");
  if (it != args.end()) {
    one_time_string = args.at("state");
  }

  if (one_time_string.empty()) {
    BLOG(0, "One time string is empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (current_one_time != one_time_string) {
    BLOG(0, "One time string mismatch");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (current_code_verifier.empty()) {
    BLOG(0, "Code verifier is empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  const std::string hashed_payment_id =
      crypto::SHA256HashString(wallet->payment_id);
  const std::string external_account_id =
      base::HexEncode(hashed_payment_id.data(), hashed_payment_id.size());

  auto url_callback = std::bind(&BitflyerAuthorization::OnAuthorize, this, _1,
                                _2, _3, _4, callback);

  bitflyer_server_->post_oauth()->Request(external_account_id, code,
                                          current_code_verifier, url_callback);
}

void BitflyerAuthorization::OnAuthorize(
    const type::Result result,
    const std::string& token,
    const std::string& address,
    const std::string& linking_info,
    ledger::ExternalWalletAuthorizationCallback callback) {
  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(type::Result::EXPIRED_TOKEN, {});
    ledger_->bitflyer()->DisconnectWallet();
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get token");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (token.empty()) {
    BLOG(0, "Token is empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (address.empty()) {
    BLOG(0, "Address is empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (linking_info.empty()) {
    BLOG(0, "Linking info is empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  auto url_callback = std::bind(&BitflyerAuthorization::OnClaimWallet, this, _1,
                                token, address, linking_info, callback);

  promotion_server_->post_claim_bitflyer()->Request(linking_info, url_callback);
}

void BitflyerAuthorization::OnClaimWallet(
    const type::Result result,
    const std::string& token,
    const std::string& address,
    const std::string& linking_info,
    ledger::ExternalWalletAuthorizationCallback callback) {
  auto wallet_ptr = ledger_->bitflyer()->GetWallet();
  if (!wallet_ptr) {
    BLOG(0, "bitFlyer wallet is null!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  const std::string event_text = std::string(constant::kWalletBitflyer) +
                                 (!wallet_ptr->address.empty() ? "/" : "") +
                                 wallet_ptr->address.substr(0, 5);

  if (result == type::Result::ALREADY_EXISTS) {
    BLOG(0, "Wallet linking limit reached!");

    ledger_->ledger_client()->ShowNotification(
        ledger::notifications::kWalletDeviceLimitReached, {},
        [](type::Result) {});

    ledger_->database()->SaveEventLog(log::kDeviceLimitReached, event_text);

    return callback(type::Result::ALREADY_EXISTS, {});
  }

  if (result == type::Result::TOO_MANY_RESULTS) {
    BLOG(0, "Mismatched bitFlyer accounts!");

    ledger_->database()->SaveEventLog(log::kMismatchedProviderAccounts,
                                      event_text);

    return callback(type::Result::TOO_MANY_RESULTS, {});
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't claim wallet");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  wallet_ptr->token = token;
  wallet_ptr->address = address;

  switch (wallet_ptr->status) {
    case type::WalletStatus::NOT_CONNECTED:
    case type::WalletStatus::DISCONNECTED_NOT_VERIFIED:
    case type::WalletStatus::DISCONNECTED_VERIFIED:
      wallet_ptr->status = type::WalletStatus::VERIFIED;
      break;
    default:
      break;
  }

  ledger_->bitflyer()->SetWallet(wallet_ptr->Clone());

  callback(type::Result::LEDGER_OK, {});
}

}  // namespace bitflyer
}  // namespace ledger
