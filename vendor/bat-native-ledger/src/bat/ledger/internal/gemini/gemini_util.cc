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
#include "bat/ledger/buildflags.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "crypto/random.h"

namespace ledger {
namespace gemini {

std::string GetClientId() {
  return ledger::_environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(GEMINI_WALLET_CLIENT_ID)
             : BUILDFLAG(GEMINI_WALLET_STAGING_CLIENT_ID);
}

std::string GetClientSecret() {
  return ledger::_environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(GEMINI_WALLET_CLIENT_SECRET)
             : BUILDFLAG(GEMINI_WALLET_STAGING_CLIENT_SECRET);
}

std::string GetUrl() {
  return ledger::_environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(GEMINI_OAUTH_URL)
             : BUILDFLAG(GEMINI_OAUTH_STAGING_URL);
}

std::string GetFeeAddress() {
  return ledger::_environment == mojom::Environment::PRODUCTION
             ? kFeeAddressProduction
             : kFeeAddressStaging;
}

std::string GetLoginUrl(const std::string& state) {
  const std::string id = GetClientId();
  const std::string url = GetUrl();

  return base::StringPrintf(
      "%s/auth"
      "?client_id=%s"
      "&scope="
      "balances:read,"
      "history:read,"
      "crypto:send,"
      "account:read,"
      "payments:create,"
      "payments:send,"
      "&redirect_uri=rewards://gemini/authorization"
      "&state=%s"
      "&response_type=code",
      url.c_str(), id.c_str(), state.c_str());
}

std::string GetAccountUrl() {
  return GetUrl();
}

std::string GetActivityUrl() {
  return base::StringPrintf("%s/balances", GetUrl().c_str());
}

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr wallet) {
  if (!wallet) {
    return nullptr;
  }

  wallet->account_url = GetAccountUrl();
  wallet->activity_url = "";
  wallet->login_url = GetLoginUrl(wallet->one_time_string);

  if (wallet->status == mojom::WalletStatus::kConnected) {
    wallet->activity_url = GetActivityUrl();
  }

  return wallet;
}

}  // namespace gemini
}  // namespace ledger
