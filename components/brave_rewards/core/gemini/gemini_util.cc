/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/gemini/gemini_util.h"

#include "base/base64.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace {
enum class UrlType { kOAuth, kAPI };

std::string GetUrl(UrlType type) {
  if (type == UrlType::kOAuth) {
    return _environment == mojom::Environment::PRODUCTION
               ? BUILDFLAG(GEMINI_PRODUCTION_OAUTH_URL)
               : BUILDFLAG(GEMINI_SANDBOX_OAUTH_URL);
  } else {
    DCHECK(type == UrlType::kAPI);
    return _environment == mojom::Environment::PRODUCTION
               ? BUILDFLAG(GEMINI_PRODUCTION_API_URL)
               : BUILDFLAG(GEMINI_SANDBOX_API_URL);
  }
}

std::string GetAccountUrl() {
  return GetUrl(UrlType::kOAuth);
}

std::string GetActivityUrl() {
  return GetUrl(UrlType::kOAuth) + "/balances";
}

std::string GetLoginUrl(const std::string& state) {
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
      GetUrl(UrlType::kOAuth).c_str(), gemini::GetClientId().c_str(),
      state.c_str());
}
}  // namespace

namespace gemini {

std::string GetClientId() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(GEMINI_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(GEMINI_SANDBOX_CLIENT_ID);
}

std::string GetClientSecret() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(GEMINI_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(GEMINI_SANDBOX_CLIENT_SECRET);
}

std::string GetFeeAddress() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(GEMINI_PRODUCTION_FEE_ADDRESS)
             : BUILDFLAG(GEMINI_SANDBOX_FEE_ADDRESS);
}

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr wallet) {
  if (wallet) {
    wallet->account_url = GetAccountUrl();
    wallet->activity_url = wallet->status == mojom::WalletStatus::kConnected
                               ? GetActivityUrl()
                               : "";
    wallet->login_url = GetLoginUrl(wallet->one_time_string);
  }

  return wallet;
}

}  // namespace gemini
}  // namespace brave_rewards::internal

namespace brave_rewards::internal::endpoint::gemini {

std::vector<std::string> RequestAuthorization(const std::string& token) {
  std::vector<std::string> headers;

  if (!token.empty()) {
    headers.push_back("Authorization: Bearer " + token);
  } else {
    std::string user;
    base::Base64Encode(
        base::StringPrintf("%s:%s", internal::gemini::GetClientId().c_str(),
                           internal::gemini::GetClientSecret().c_str()),
        &user);
    headers.push_back("Authorization: Basic " + user);
  }

  return headers;
}

std::string GetApiServerUrl(const std::string& path) {
  DCHECK(!path.empty());
  return GetUrl(UrlType::kAPI) + path;
}

std::string GetOauthServerUrl(const std::string& path) {
  DCHECK(!path.empty());
  return GetUrl(UrlType::kOAuth) + path;
}

mojom::Result CheckStatusCode(int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED ||
      status_code == net::HTTP_FORBIDDEN) {
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    return mojom::Result::NOT_FOUND;
  }

  if (status_code != net::HTTP_OK) {
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

}  // namespace brave_rewards::internal::endpoint::gemini
