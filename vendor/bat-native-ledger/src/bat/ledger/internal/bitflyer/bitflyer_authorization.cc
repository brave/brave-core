/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bitflyer/bitflyer_authorization.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/endpoint/bitflyer/bitflyer_server.h"
#include "bat/ledger/internal/endpoints/post_connect/bitflyer/post_connect_bitflyer.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/logging/event_log_util.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
#include "crypto/sha2.h"

using ledger::endpoints::PostConnect;
using ledger::endpoints::PostConnectBitflyer;
using ledger::endpoints::RequestFor;
using ledger::wallet::OnWalletStatusChange;

namespace ledger::bitflyer {

BitflyerAuthorization::BitflyerAuthorization(LedgerImpl* ledger)
    : ledger_(ledger),
      bitflyer_server_(std::make_unique<endpoint::BitflyerServer>(ledger)) {}

BitflyerAuthorization::~BitflyerAuthorization() = default;

void BitflyerAuthorization::Authorize(
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  auto bitflyer_wallet = ledger_->bitflyer()->GetWallet();
  if (!bitflyer_wallet) {
    BLOG(0, "Wallet is null");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  const auto current_one_time = bitflyer_wallet->one_time_string;
  const auto current_code_verifier = bitflyer_wallet->code_verifier;

  // We need to generate new strings as soon as authorization is triggered
  bitflyer_wallet->one_time_string = util::GenerateRandomHexString();
  bitflyer_wallet->code_verifier = util::GeneratePKCECodeVerifier();
  const bool success = ledger_->bitflyer()->SetWallet(bitflyer_wallet->Clone());

  if (!success) {
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  auto it = args.find("error_description");
  if (it != args.end()) {
    const std::string message = args.at("error_description");
    BLOG(1, message);
    if (message == "User does not meet minimum requirements.") {
      ledger_->database()->SaveEventLog(log::kKYCRequired,
                                        constant::kWalletBitflyer);
      callback(mojom::Result::NOT_FOUND, {});
      return;
    }

    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  if (args.empty()) {
    BLOG(0, "Arguments are empty");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  std::string code;
  it = args.find("code");
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

  if (one_time_string.empty()) {
    BLOG(0, "One time string is empty");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  if (current_one_time != one_time_string) {
    BLOG(0, "One time string mismatch");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  if (current_code_verifier.empty()) {
    BLOG(0, "Code verifier is empty");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  const std::string hashed_payment_id =
      crypto::SHA256HashString(wallet->payment_id);
  const std::string external_account_id =
      base::HexEncode(hashed_payment_id.data(), hashed_payment_id.size());

  bitflyer_server_->post_oauth()->Request(
      external_account_id, code, current_code_verifier,
      base::BindOnce(&BitflyerAuthorization::OnAuthorize,
                     base::Unretained(this), std::move(callback)));
}

void BitflyerAuthorization::OnAuthorize(
    ledger::ExternalWalletAuthorizationCallback callback,
    mojom::Result result,
    std::string&& token,
    std::string&& address,
    std::string&& linking_info) {
  if (result == mojom::Result::EXPIRED_TOKEN) {
    BLOG(0, "Expired token");
    callback(mojom::Result::EXPIRED_TOKEN, {});
    ledger_->bitflyer()->DisconnectWallet();
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

  if (address.empty()) {
    BLOG(0, "Address is empty");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  if (linking_info.empty()) {
    BLOG(0, "Linking info is empty");
    callback(mojom::Result::LEDGER_ERROR, {});
    return;
  }

  auto on_connect = base::BindOnce(&BitflyerAuthorization::OnConnectWallet,
                                   base::Unretained(this), std::move(callback),
                                   std::move(token), std::move(address));

  RequestFor<PostConnectBitflyer>(ledger_, std::move(linking_info))
      .Send(std::move(on_connect));
}

void BitflyerAuthorization::OnConnectWallet(
    ledger::ExternalWalletAuthorizationCallback callback,
    std::string&& token,
    std::string&& address,
    PostConnect::Result&& result) {
  const auto legacy_result = PostConnect::ToLegacyResult(result);

  auto wallet_ptr = ledger_->bitflyer()->GetWallet();
  if (!wallet_ptr) {
    BLOG(0, "bitFlyer wallet is null!");
    return callback(mojom::Result::LEDGER_ERROR, {});
  }

  DCHECK(!token.empty());
  DCHECK(!address.empty());
  const std::string abbreviated_address = address.substr(0, 5);

  switch (legacy_result) {
    case mojom::Result::DEVICE_LIMIT_REACHED:
    case mojom::Result::MISMATCHED_PROVIDER_ACCOUNTS:
    case mojom::Result::REQUEST_SIGNATURE_VERIFICATION_FAILURE:
    case mojom::Result::FLAGGED_WALLET:
    case mojom::Result::REGION_NOT_SUPPORTED:
    case mojom::Result::MISMATCHED_PROVIDER_ACCOUNT_REGIONS:
      ledger_->database()->SaveEventLog(
          log::GetEventLogKeyForLinkingResult(legacy_result),
          constant::kWalletBitflyer + std::string("/") + abbreviated_address);
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
  wallet_ptr->address = std::move(address);

  if (!ledger_->bitflyer()->SetWallet(std::move(wallet_ptr))) {
    BLOG(0, "Unable to set bitFlyer wallet!");
    return callback(mojom::Result::LEDGER_ERROR, {});
  }

  OnWalletStatusChange(ledger_, from, to);
  ledger_->database()->SaveEventLog(
      log::kWalletVerified,
      constant::kWalletBitflyer + std::string("/") + abbreviated_address);
  callback(mojom::Result::LEDGER_OK, {});
}

}  // namespace ledger::bitflyer
