/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"

#include "base/base64.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::internal {
namespace {
std::string GetUrl() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(BITFLYER_PRODUCTION_URL)
             : BUILDFLAG(BITFLYER_SANDBOX_URL);
}

std::string GetAccountUrl() {
  return GetUrl() + "/ex/Home?login=1";
}

std::string GetActivityUrl() {
  return GetUrl() + "/ja-jp/ex/tradehistory";
}

std::string GetLoginUrl(const std::string& state,
                        const std::string& code_verifier) {
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
      GetUrl().c_str(), bitflyer::GetClientId().c_str(), state.c_str(),
      util::GeneratePKCECodeChallenge(code_verifier).c_str());
}
}  // namespace

namespace bitflyer {

std::string GetClientId() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(BITFLYER_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(BITFLYER_SANDBOX_CLIENT_ID);
}

std::string GetClientSecret() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(BITFLYER_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(BITFLYER_SANDBOX_CLIENT_SECRET);
}

std::string GetFeeAddress() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(BITFLYER_PRODUCTION_FEE_ADDRESS)
             : BUILDFLAG(BITFLYER_SANDBOX_FEE_ADDRESS);
}

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr wallet) {
  if (wallet) {
    wallet->account_url = GetAccountUrl();
    wallet->activity_url = wallet->status == mojom::WalletStatus::kConnected
                               ? GetActivityUrl()
                               : "";
    wallet->login_url =
        GetLoginUrl(wallet->one_time_string, wallet->code_verifier);
  }

  return wallet;
}

}  // namespace bitflyer
}  // namespace brave_rewards::internal

namespace brave_rewards::internal::endpoint::bitflyer {

std::vector<std::string> RequestAuthorization(const std::string& token) {
  std::vector<std::string> headers;

  if (!token.empty()) {
    headers.push_back("Authorization: Bearer " + token);
  } else {
    std::string user;
    base::Base64Encode(
        base::StringPrintf("%s:%s", internal::bitflyer::GetClientId().c_str(),
                           internal::bitflyer::GetClientSecret().c_str()),
        &user);
    headers.push_back("Authorization: Basic " + user);
  }

  return headers;
}

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());
  return GetUrl() + path;
}

}  // namespace brave_rewards::internal::endpoint::bitflyer
