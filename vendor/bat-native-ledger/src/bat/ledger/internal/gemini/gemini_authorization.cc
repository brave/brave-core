/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/gemini/gemini_authorization.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/notifications/notification_keys.h"
#include "crypto/sha2.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace ledger {
namespace gemini {

GeminiAuthorization::GeminiAuthorization(LedgerImpl* ledger)
    : ledger_(ledger),
      gemini_server_(std::make_unique<endpoint::GeminiServer>(ledger)),
      promotion_server_(std::make_unique<endpoint::PromotionServer>(ledger)) {}

GeminiAuthorization::~GeminiAuthorization() = default;

void GeminiAuthorization::Authorize(
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  auto gemini_wallet = ledger_->gemini()->GetWallet();
  if (!gemini_wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  const auto current_one_time = gemini_wallet->one_time_string;

  // We need to generate new strings as soon as authorization is triggered
  gemini_wallet->one_time_string = util::GenerateRandomHexString();
  const bool success = ledger_->gemini()->SetWallet(gemini_wallet->Clone());

  if (!success) {
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (args.empty()) {
    BLOG(0, "Arguments are empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  std::string code;
  auto it = args.find("code");
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

  if (one_time_string.empty() || one_time_string.length() != 64) {
    BLOG(0, "One time string is empty");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  if (current_one_time != one_time_string) {
    BLOG(0, "One time string mismatch");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  const std::string hashed_payment_id =
      crypto::SHA256HashString(wallet->payment_id);
  const std::string external_account_id =
      base::HexEncode(hashed_payment_id.data(), hashed_payment_id.size());

  auto url_callback =
      std::bind(&GeminiAuthorization::OnAuthorize, this, _1, _2, callback);

  gemini_server_->post_oauth()->Request(external_account_id, code,
                                        url_callback);
}

void GeminiAuthorization::OnAuthorize(
    const type::Result result,
    const std::string& token,
    ledger::ExternalWalletAuthorizationCallback callback) {
  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    ledger_->gemini()->DisconnectWallet();
    callback(type::Result::EXPIRED_TOKEN, {});
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
  auto url_callback = std::bind(&GeminiAuthorization::OnFetchRecipientId, this,
                                _1, _2, token, callback);
  gemini_server_->post_recipient_id()->Request(token, url_callback);
}

void GeminiAuthorization::OnFetchRecipientId(
    const type::Result result,
    const std::string& recipient_id,
    const std::string& token,
    ledger::ExternalWalletAuthorizationCallback callback) {
  if (result == type::Result::NOT_FOUND) {
    BLOG(0, "Unverified User");
    callback(type::Result::NOT_FOUND, {});
    return;
  }

  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(type::Result::EXPIRED_TOKEN, {});
    ledger_->gemini()->DisconnectWallet();
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get token");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  auto wallet_ptr = ledger_->gemini()->GetWallet();
  wallet_ptr->address = recipient_id;
  ledger_->gemini()->SetWallet(wallet_ptr->Clone());

  auto url_callback = std::bind(&GeminiAuthorization::OnPostAccount, this, _1,
                                _2, _3, token, callback);
  gemini_server_->post_account()->Request(token, url_callback);
}

void GeminiAuthorization::OnPostAccount(
    const type::Result result,
    const std::string& linking_info,
    const std::string& name,
    const std::string& token,
    ledger::ExternalWalletAuthorizationCallback callback) {
  if (result == type::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(type::Result::EXPIRED_TOKEN, {});
    ledger_->gemini()->DisconnectWallet();
    return;
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get token");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  auto wallet_ptr = ledger_->gemini()->GetWallet();

  wallet_ptr->token = token;
  wallet_ptr->user_name = name;

  ledger_->gemini()->SetWallet(wallet_ptr->Clone());

  auto url_callback =
      std::bind(&GeminiAuthorization::OnClaimWallet, this, _1, token, callback);
  promotion_server_->post_claim_gemini()->Request(
      linking_info, wallet_ptr->address, url_callback);
}

void GeminiAuthorization::OnClaimWallet(
    const type::Result result,
    const std::string& token,
    ledger::ExternalWalletAuthorizationCallback callback) {
  auto wallet_ptr = ledger_->gemini()->GetWallet();
  if (!wallet_ptr) {
    BLOG(0, "Gemini wallet is null!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  const std::string event_text = std::string(constant::kWalletGemini) +
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
    BLOG(0, "Mismatched Gemini accounts!");

    ledger_->database()->SaveEventLog(log::kMismatchedProviderAccounts,
                                      event_text);

    return callback(type::Result::TOO_MANY_RESULTS, {});
  }

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't claim wallet");
    callback(type::Result::LEDGER_ERROR, {});
    return;
  }

  switch (wallet_ptr->status) {
    case type::WalletStatus::NOT_CONNECTED:
    case type::WalletStatus::DISCONNECTED_NOT_VERIFIED:
    case type::WalletStatus::DISCONNECTED_VERIFIED:
      wallet_ptr->status = type::WalletStatus::VERIFIED;
      break;
    default:
      break;
  }

  ledger_->gemini()->SetWallet(wallet_ptr->Clone());
  callback(type::Result::LEDGER_OK, {});
}

}  // namespace gemini
}  // namespace ledger
