/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/uphold/uphold_authorization.h"

#include <utility>

#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace uphold {

UpholdAuthorization::UpholdAuthorization(LedgerImpl* ledger)
    : ledger_(ledger),
      uphold_server_(std::make_unique<endpoint::UpholdServer>(ledger)) {}

UpholdAuthorization::~UpholdAuthorization() = default;

void UpholdAuthorization::Authorize(
    const base::flat_map<std::string, std::string>& args,
    ledger::ExternalWalletAuthorizationCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  if (uphold_wallet->status != type::WalletStatus::NOT_CONNECTED &&
      uphold_wallet->status != type::WalletStatus::DISCONNECTED_VERIFIED) {
    BLOG(0, "Attempting to re-authorize in " << uphold_wallet->status
                                             << " status!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  DCHECK(uphold_wallet->token.empty());
  DCHECK(uphold_wallet->address.empty());

  const auto current_one_time = uphold_wallet->one_time_string;

  // we need to generate new string as soon as authorization is triggered
  uphold_wallet->one_time_string = util::GenerateRandomHexString();
  if (!ledger_->uphold()->SetWallet(uphold_wallet->Clone())) {
    BLOG(0, "Unable to set the Uphold wallet!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  auto it = args.find("error_description");
  if (it != args.end()) {
    const std::string message = args.at("error_description");
    BLOG(1, message);
    if (message == "User does not meet minimum requirements") {
      return callback(type::Result::NOT_FOUND, {});
    }

    return callback(type::Result::LEDGER_ERROR, {});
  }

  if (args.empty()) {
    BLOG(0, "Arguments are empty!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  std::string code;
  it = args.find("code");
  if (it != args.end()) {
    code = args.at("code");
  }

  if (code.empty()) {
    BLOG(0, "code is empty!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  std::string one_time_string;
  it = args.find("state");
  if (it != args.end()) {
    one_time_string = args.at("state");
  }

  if (one_time_string.empty()) {
    BLOG(0, "state is empty!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  if (current_one_time != one_time_string) {
    BLOG(0, "One-time string mismatch!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  uphold_server_->post_oauth()->Request(
      code,
      std::bind(&UpholdAuthorization::OnAuthorize, this, _1, _2, callback));
}

void UpholdAuthorization::OnAuthorize(
    const type::Result result,
    const std::string& token,
    ledger::ExternalWalletAuthorizationCallback callback) const {
  auto uphold_wallet = ledger_->uphold()->GetWallet();
  if (!uphold_wallet) {
    BLOG(0, "Uphold wallet is null!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  if (uphold_wallet->status != type::WalletStatus::NOT_CONNECTED &&
      uphold_wallet->status != type::WalletStatus::DISCONNECTED_VERIFIED) {
    BLOG(0, "Attempting to re-authorize in " << uphold_wallet->status
                                             << " status!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  DCHECK(uphold_wallet->token.empty());
  DCHECK(uphold_wallet->address.empty());

  if (result != type::Result::LEDGER_OK) {
    BLOG(0, "Couldn't exchange code for the access token!");
    return callback(result, {});
  }

  if (token.empty()) {
    BLOG(0, "Access token is empty!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  const auto from = uphold_wallet->status;
  const auto to = uphold_wallet->status = type::WalletStatus::PENDING;
  uphold_wallet->token = token;
  uphold_wallet = GenerateLinks(std::move(uphold_wallet));
  if (!ledger_->uphold()->SetWallet(std::move(uphold_wallet))) {
    BLOG(0, "Unable to set the Uphold wallet!");
    return callback(type::Result::LEDGER_ERROR, {});
  }

  OnWalletStatusChange(ledger_, from, to);

  callback(type::Result::LEDGER_OK, {});
}

}  // namespace uphold
}  // namespace ledger
