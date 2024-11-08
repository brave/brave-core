/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/identity/brave_web_auth_flow.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "chrome/browser/extensions/api/identity/identity_api.h"
#include "chrome/browser/profiles/profile.h"
#include "google_apis/google_api_keys.h"
#include "net/base/url_util.h"

namespace extensions {

namespace {
constexpr char kGoogleOauth2Url[] =
    "https://accounts.google.com/o/oauth2/v2/auth";
}  // namespace

BraveWebAuthFlow::BraveWebAuthFlow() {}
BraveWebAuthFlow::~BraveWebAuthFlow() {}

// static
std::optional<std::string> BraveWebAuthFlow::token_for_testing_;
// static
void BraveWebAuthFlow::SetTokenForTesting(const std::string& token) {
  token_for_testing_ = token;
}

void BraveWebAuthFlow::StartWebAuthFlow(
    Profile* profile,
    base::OnceClosure complete_mint_token_flow_callback,
    CompleteFunctionWithErrorCallback complete_with_error_callback,
    CompleteFunctionWithResultCallback complete_with_result_callback,
    const std::string& oauth2_client_id,
    ExtensionTokenKey token_key,
    bool interactive,
    bool user_gesture) {
  profile_ = profile;
  complete_with_error_callback_ = std::move(complete_with_error_callback);
  complete_with_result_callback_ = std::move(complete_with_result_callback);
  complete_mint_token_flow_callback_ =
      std::move(complete_mint_token_flow_callback);
  token_key_ = token_key;

  if (token_for_testing_.has_value()) {
    std::move(complete_mint_token_flow_callback_).Run();
    std::move(complete_with_result_callback_)
        .Run(token_for_testing_.value(), token_key_.scopes);
    return;
  }
  // Compute the reverse DNS notation of the client ID and use it as a custom
  // URI scheme.
  std::vector<std::string> client_id_components = base::SplitString(
      oauth2_client_id, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  std::reverse(client_id_components.begin(), client_id_components.end());
  GURL redirect_url = GURL(base::JoinString(client_id_components, ".") + ":/");
  redirect_scheme_ = redirect_url.scheme();
  GURL google_oauth_url = GURL(kGoogleOauth2Url);
  google_oauth_url = net::AppendQueryParameter(google_oauth_url, "client_id",
                                               oauth2_client_id);
  google_oauth_url = net::AppendQueryParameter(google_oauth_url, "redirect_uri",
                                               redirect_url.spec());
  google_oauth_url =
      net::AppendQueryParameter(google_oauth_url, "response_type", "token");
  google_oauth_url = net::AppendQueryParameter(
      google_oauth_url, "scope",
      base::JoinString(std::vector<std::string>(token_key_.scopes.begin(),
                                                token_key_.scopes.end()),
                       " "));
  web_auth_flow_ = std::make_unique<WebAuthFlow>(
      this, profile_, google_oauth_url,
      interactive ? WebAuthFlow::INTERACTIVE : WebAuthFlow::SILENT,
      user_gesture);
  web_auth_flow_->Start();
}

void BraveWebAuthFlow::OnAuthFlowFailure(WebAuthFlow::Failure failure) {
  IdentityGetAuthTokenError error;
  switch (failure) {
    case WebAuthFlow::WINDOW_CLOSED:
      error = IdentityGetAuthTokenError(
          IdentityGetAuthTokenError::State::kRemoteConsentFlowRejected);
      break;
    case WebAuthFlow::INTERACTION_REQUIRED:
      error = IdentityGetAuthTokenError(
          IdentityGetAuthTokenError::State::kGaiaConsentInteractionRequired);
      break;
    case WebAuthFlow::LOAD_FAILED:
      error = IdentityGetAuthTokenError(
          IdentityGetAuthTokenError::State::kRemoteConsentPageLoadFailure);
      break;
    default:
      NOTREACHED() << "Unexpected error from web auth flow: " << failure;
  }
  if (web_auth_flow_) {
    web_auth_flow_.release()->DetachDelegateAndDelete();
  }
  std::move(complete_mint_token_flow_callback_).Run();
  std::move(complete_with_error_callback_).Run(error);
}

void BraveWebAuthFlow::OnAuthFlowURLChange(const GURL& redirect_url) {
  if (!redirect_url.SchemeIs(redirect_scheme_)) {
    return;
  }

  if (web_auth_flow_) {
    web_auth_flow_.release()->DetachDelegateAndDelete();
  }

  std::move(complete_mint_token_flow_callback_).Run();

  base::StringPairs response_pairs;
  if (!base::SplitStringIntoKeyValuePairs(redirect_url.ref(), '=', '&',
                                          &response_pairs)) {
    std::move(complete_with_error_callback_)
        .Run(IdentityGetAuthTokenError(
            IdentityGetAuthTokenError::State::kNoGrant));
    return;
  }

  auto access_token_it =
      std::find_if(response_pairs.begin(), response_pairs.end(),
                   [](const auto& key_value_pair) {
                     return key_value_pair.first == "access_token";
                   });
  if (access_token_it == response_pairs.end()) {
    std::move(complete_with_error_callback_)
        .Run(IdentityGetAuthTokenError(
            IdentityGetAuthTokenError::State::kNoGrant));
    return;
  }
  std::string access_token = access_token_it->second;

  auto expires_in_it =
      std::find_if(response_pairs.begin(), response_pairs.end(),
                   [](const auto& key_value_pair) {
                     return key_value_pair.first == "expires_in";
                   });
  int time_to_live_seconds;
  if (expires_in_it == response_pairs.end() ||
      !base::StringToInt(expires_in_it->second, &time_to_live_seconds)) {
    std::move(complete_with_error_callback_)
        .Run(IdentityGetAuthTokenError(
            IdentityGetAuthTokenError::State::kNoGrant));
    return;
  }

  // `token_key_` doesn't have information about the account being used, so only
  // the last used token will be cached.
  IdentityTokenCacheValue token = IdentityTokenCacheValue::CreateToken(
      access_token, token_key_.scopes, base::Seconds(time_to_live_seconds));
  IdentityAPI::GetFactoryInstance()->Get(profile_)->token_cache()->SetToken(
      token_key_, token);

  std::move(complete_with_result_callback_)
      .Run(access_token, token_key_.scopes);
}

}  // namespace extensions
