/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/zebpay/zebpay_util.h"

#include "base/base64.h"
#include "base/strings/escape.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace {
enum class UrlType { kOAuth, kAPI };

std::string GetUrl(UrlType type) {
  if (type == UrlType::kOAuth) {
    return _environment == mojom::Environment::PRODUCTION
               ? BUILDFLAG(ZEBPAY_PRODUCTION_OAUTH_URL)
               : BUILDFLAG(ZEBPAY_SANDBOX_OAUTH_URL);
  } else {
    DCHECK(type == UrlType::kAPI);
    return _environment == mojom::Environment::PRODUCTION
               ? BUILDFLAG(ZEBPAY_PRODUCTION_API_URL)
               : BUILDFLAG(ZEBPAY_SANDBOX_API_URL);
  }
}

}  // namespace

namespace zebpay {

std::string GetLoginUrl(const std::string& state) {
  return GetUrl(UrlType::kOAuth) + "/account/login?returnUrl=" +
         // ZebPay requires almost the entire URL to be escaped.
         base::EscapeQueryParamValue(
             base::StringPrintf("/connect/authorize/callback"
                                "?client_id=%s"
                                "&grant_type=authorization_code"
                                "&redirect_uri=rewards://zebpay/authorization"
                                "&response_type=code"
                                "&scope="
                                "openid "
                                "profile"
                                "&state=%s",
                                zebpay::GetClientId().c_str(), state.c_str()),
             false);
}

std::string GetClientId() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(ZEBPAY_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(ZEBPAY_SANDBOX_CLIENT_ID);
}

std::string GetClientSecret() {
  return _environment == mojom::Environment::PRODUCTION
             ? BUILDFLAG(ZEBPAY_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(ZEBPAY_SANDBOX_CLIENT_SECRET);
}

std::string GetAccountUrl() {
  return GetUrl(UrlType::kAPI) + "/dashboard";
}

std::string GetActivityUrl() {
  return GetUrl(UrlType::kAPI) + "/activity";
}

}  // namespace zebpay
}  // namespace brave_rewards::internal

namespace brave_rewards::internal::endpoint::zebpay {

std::vector<std::string> RequestAuthorization(const std::string& token) {
  std::vector<std::string> headers;

  if (!token.empty()) {
    headers.push_back("Authorization: Bearer " + token);
  } else {
    std::string user;
    base::Base64Encode(
        base::StringPrintf("%s:%s", internal::zebpay::GetClientId().c_str(),
                           internal::zebpay::GetClientSecret().c_str()),
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

}  // namespace brave_rewards::internal::endpoint::zebpay
