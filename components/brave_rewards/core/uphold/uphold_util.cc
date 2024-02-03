/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/uphold/uphold_util.h"

#include "base/base64.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace {
enum class UrlType { kOAuth, kAPI };

std::string GetUrl(UrlType type) {
  if (type == UrlType::kOAuth) {
    return _environment == mojom::Environment::PRODUCTION
               ? BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL)
               : BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL);
  } else {
    DCHECK(type == UrlType::kAPI);
    return _environment == mojom::Environment::PRODUCTION
               ? BUILDFLAG(UPHOLD_PRODUCTION_API_URL)
               : BUILDFLAG(UPHOLD_SANDBOX_API_URL);
  }
}

}  // namespace

namespace uphold {

std::string GetLoginUrl(const std::string& state) {
  return base::StringPrintf(
      "%s/authorize/%s"
      "?scope="
      "cards:read "
      "cards:write "
      "user:read "
      "transactions:read "
      "transactions:transfer:application "
      "transactions:transfer:others"
      "&intention=login&"
      "state=%s",
      GetUrl(UrlType::kOAuth).c_str(), uphold::GetClientId().c_str(),
      state.c_str());
}

std::string GetClientId() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(UPHOLD_SANDBOX_CLIENT_ID);
}

std::string GetClientSecret() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(UPHOLD_SANDBOX_CLIENT_SECRET);
}

std::string GetFeeAddress() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(UPHOLD_PRODUCTION_FEE_ADDRESS)
             : BUILDFLAG(UPHOLD_SANDBOX_FEE_ADDRESS);
}

std::string GetAccountUrl() {
  return GetUrl(UrlType::kOAuth) + "/dashboard";
}

std::string GetActivityUrl(const std::string& address) {
  DCHECK(!address.empty());
  return base::StringPrintf("%s/dashboard/cards/%s/activity",
                            GetUrl(UrlType::kOAuth).c_str(), address.c_str());
}

}  // namespace uphold
}  // namespace brave_rewards::internal

namespace brave_rewards::internal::endpoint::uphold {

std::vector<std::string> RequestAuthorization(const std::string& token) {
  std::vector<std::string> headers;

  if (!token.empty()) {
    headers.push_back("Authorization: Bearer " + token);
  } else {
    std::string user;
    base::Base64Encode(
        base::StringPrintf("%s:%s", internal::uphold::GetClientId().c_str(),
                           internal::uphold::GetClientSecret().c_str()),
        &user);
    headers.push_back("Authorization: Basic " + user);
  }

  return headers;
}

std::string GetServerUrl(const std::string& path) {
  DCHECK(!path.empty());
  return GetUrl(UrlType::kAPI) + path;
}

}  // namespace brave_rewards::internal::endpoint::uphold
