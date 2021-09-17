/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "crypto/random.h"

namespace ledger {
namespace bitflyer {

std::string GetClientId() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? BITFLYER_CLIENT_ID
             : BITFLYER_STAGING_CLIENT_ID;
}

std::string GetClientSecret() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? BITFLYER_CLIENT_SECRET
             : BITFLYER_STAGING_CLIENT_SECRET;
}

std::string GetUrl() {
  return ledger::_environment == type::Environment::PRODUCTION ? kUrlProduction
                                                               : kUrlStaging;
}

std::string GetFeeAddress() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? kFeeAddressProduction
             : kFeeAddressStaging;
}

std::string GetACAddress() {
  return ledger::_environment == type::Environment::PRODUCTION
             ? kACAddressProduction
             : kACAddressStaging;
}

std::string GetAuthorizeUrl(const std::string& state,
                            const std::string& code_verifier) {
  const std::string id = GetClientId();
  const std::string url = GetUrl();

  // Calculate PKCE code challenge
  std::string code_challenge = util::GeneratePKCECodeChallenge(code_verifier);
  return base::StringPrintf(
      "%s/ex/OAuth/authorize"
      "?client_id=%s"
      "&scope="
      "assets "
      "create_deposit_id "
      "withdraw_to_deposit_id"
      "&redirect_uri=rewards://bitflyer/authorization"
      "&state=%s"
      "&response_type=code"
      "&code_challenge_method=S256"
      "&code_challenge=%s",
      url.c_str(), id.c_str(), state.c_str(), code_challenge.c_str());
}

std::string GetAddUrl() {
  return GetAccountUrl();
}

std::string GetWithdrawUrl() {
  return GetAccountUrl();
}

std::string GetAccountUrl() {
  const std::string url = GetUrl();

  return base::StringPrintf("%s/ex/Home?login=1", url.c_str());
}

std::string GetActivityUrl() {
  return base::StringPrintf("%s/ja-jp/ex/tradehistory", GetUrl().c_str());
}

type::ExternalWalletPtr GenerateLinks(type::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  switch (wallet->status) {
    case type::WalletStatus::VERIFIED: {
      wallet->add_url = GetAddUrl();
      wallet->withdraw_url = GetWithdrawUrl();
      break;
    }
    case type::WalletStatus::CONNECTED:
    case type::WalletStatus::PENDING:
    case type::WalletStatus::NOT_CONNECTED:
    case type::WalletStatus::DISCONNECTED_VERIFIED:
    case type::WalletStatus::DISCONNECTED_NOT_VERIFIED: {
      wallet->add_url = "";
      wallet->withdraw_url = "";
      break;
    }
  }

  const std::string auth_url =
      GetAuthorizeUrl(wallet->one_time_string, wallet->code_verifier);

  wallet->verify_url = auth_url;
  wallet->account_url = GetAccountUrl();
  wallet->login_url = auth_url;
  wallet->activity_url = GetActivityUrl();

  return wallet;
}

}  // namespace bitflyer
}  // namespace ledger
