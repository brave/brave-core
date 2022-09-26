/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/gemini/gemini_authorization.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/endpoint/gemini/gemini_server.h"
#include "bat/ledger/internal/endpoints/post_connect/gemini/post_connect_gemini.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/logging/event_log_util.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
#include "crypto/sha2.h"

using ledger::endpoints::PostConnect;
using ledger::endpoints::PostConnectGemini;
using ledger::endpoints::RequestFor;
using ledger::wallet::OnWalletStatusChange;

namespace ledger {
namespace gemini {

GeminiAuthorization::GeminiAuthorization(LedgerImpl* ledger)
    : ledger_(ledger),
      gemini_server_(std::make_unique<endpoint::GeminiServer>(ledger)) {}

GeminiAuthorization::~GeminiAuthorization() = default;

void GeminiAuthorization::Authorize(
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  auto gemini_wallet = ledger_->gemini()->GetWallet();
  if (!gemini_wallet) {
    BLOG(0, "Wallet is null");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  const auto current_one_time = gemini_wallet->one_time_string;

  // We need to generate new strings as soon as authorization is triggered
  gemini_wallet->one_time_string = util::GenerateRandomHexString();
  const bool success = ledger_->gemini()->SetWallet(gemini_wallet->Clone());

  if (!success) {
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  if (args.empty()) {
    BLOG(0, "Arguments are empty");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  std::string code;
  auto it = args.find("code");
  if (it != args.end()) {
    code = args.at("code");
  }

  if (code.empty()) {
    BLOG(0, "Code is empty");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  std::string one_time_string;
  it = args.find("state");
  if (it != args.end()) {
    one_time_string = args.at("state");
  }

  if (one_time_string.empty() || one_time_string.length() != 64) {
    BLOG(0, "One time string is empty");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  if (current_one_time != one_time_string) {
    BLOG(0, "One time string mismatch");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  const std::string hashed_payment_id =
      crypto::SHA256HashString(wallet->payment_id);
  const std::string external_account_id =
      base::HexEncode(hashed_payment_id.data(), hashed_payment_id.size());

  gemini_server_->post_oauth()->Request(
      external_account_id, code,
      base::BindOnce(&GeminiAuthorization::OnAuthorize, base::Unretained(this),
                     std::move(callback)));
}

void GeminiAuthorization::OnAuthorize(
    ledger::ExternalWalletAuthorizationCallback callback,
    mojom::Result result,
    std::string&& token) {
  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    ledger_->gemini()->DisconnectWallet();
    callback(mojom::Result::EXPIRED_TOKEN, {});
    return;
  }

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get token");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  if (token.empty()) {
    BLOG(0, "Token is empty");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  gemini_server_->post_recipient_id()->Request(
      token,
      base::BindOnce(&GeminiAuthorization::OnFetchRecipientId,
                     base::Unretained(this), std::move(callback), token));
}

void GeminiAuthorization::OnFetchRecipientId(
    ledger::ExternalWalletAuthorizationCallback callback,
    std::string&& token,
    mojom::Result result,
    std::string&& recipient_id) {
  if (result == mojom::Result::NOT_FOUND) {
    BLOG(0, "Unverified User");
    ledger_->database()->SaveEventLog(log::kKYCRequired,
                                      constant::kWalletGemini);
    callback(mojom::Result::NOT_FOUND, {});
    return;
  }

  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(mojom::Result::EXPIRED_TOKEN, {});
    ledger_->gemini()->DisconnectWallet();
    return;
  }

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get token");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  if (recipient_id.empty()) {
    BLOG(0, "Recipient ID is empty!");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  gemini_server_->post_account()->Request(
      token, base::BindOnce(&GeminiAuthorization::OnPostAccount,
                            base::Unretained(this), std::move(callback), token,
                            std::move(recipient_id)));
}

void GeminiAuthorization::OnPostAccount(
    ledger::ExternalWalletAuthorizationCallback callback,
    std::string&& token,
    std::string&& recipient_id,
    mojom::Result result,
    std::string&& linking_info,
    std::string&& user_name) {
  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(mojom::Result::EXPIRED_TOKEN, {});
    ledger_->gemini()->DisconnectWallet();
    return;
  }

  if (result != mojom::Result::LEDGER_OK) {
    BLOG(0, "Couldn't get token");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  auto wallet_ptr = ledger_->gemini()->GetWallet();
  wallet_ptr->user_name = std::move(user_name);
  ledger_->gemini()->SetWallet(std::move(wallet_ptr));

  auto on_connect = base::BindOnce(&GeminiAuthorization::OnConnectWallet,
                                   base::Unretained(this), std::move(callback),
                                   std::move(token), recipient_id);

  RequestFor<PostConnectGemini>(ledger_, std::move(linking_info),
                                std::move(recipient_id))
      .Send(std::move(on_connect));
}

void GeminiAuthorization::OnConnectWallet(
    ledger::ExternalWalletAuthorizationCallback callback,
    std::string&& token,
    std::string&& recipient_id,
    PostConnect::Result&& result) {
  const auto legacy_result = PostConnect::ToLegacyResult(result);

  auto wallet_ptr = ledger_->gemini()->GetWallet();
  if (!wallet_ptr) {
    BLOG(0, "Gemini wallet is null!");
    return callback(mojom::Result::LEDGER_ERROR, {});
  }

  DCHECK(!token.empty());
  DCHECK(!recipient_id.empty());
  const std::string abbreviated_address = recipient_id.substr(0, 5);

  switch (legacy_result) {
    case mojom::Result::DEVICE_LIMIT_REACHED:
    case mojom::Result::MISMATCHED_PROVIDER_ACCOUNTS:
    case mojom::Result::REQUEST_SIGNATURE_VERIFICATION_FAILURE:
    case mojom::Result::FLAGGED_WALLET:
    case mojom::Result::REGION_NOT_SUPPORTED:
    case mojom::Result::MISMATCHED_PROVIDER_ACCOUNT_REGIONS:
      ledger_->database()->SaveEventLog(
          log::GetEventLogKeyForLinkingResult(legacy_result),
          constant::kWalletGemini + std::string("/") + abbreviated_address);
      return callback(legacy_result, {});
    default:
      if (legacy_result != mojom::Result::LEDGER_OK) {
        BLOG(0, "Couldn't claim wallet!");
        return callback(legacy_result, {});
      }
  }

  const auto from = wallet_ptr->status;
  const auto to = wallet_ptr->status = mojom::WalletStatus::VERIFIED;
  wallet_ptr->token = std::move(token);
  wallet_ptr->address = std::move(recipient_id);

  if (!ledger_->gemini()->SetWallet(std::move(wallet_ptr))) {
    BLOG(0, "Unable to set Gemini wallet!");
    return callback(mojom::Result::LEDGER_ERROR, {});
  }

  OnWalletStatusChange(ledger_, from, to);
  ledger_->database()->SaveEventLog(
      log::kWalletVerified,
      constant::kWalletGemini + std::string("/") + abbreviated_address);
  callback(mojom::Result::LEDGER_OK, {});
}

}  // namespace gemini
}  // namespace ledger
