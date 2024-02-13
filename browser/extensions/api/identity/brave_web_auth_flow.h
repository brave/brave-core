/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_IDENTITY_BRAVE_WEB_AUTH_FLOW_H_
#define BRAVE_BROWSER_EXTENSIONS_API_IDENTITY_BRAVE_WEB_AUTH_FLOW_H_

#include <memory>
#include <optional>
#include <set>
#include <string>

#include "base/functional/callback.h"
#include "chrome/browser/extensions/api/identity/extension_token_key.h"
#include "chrome/browser/extensions/api/identity/identity_get_auth_token_error.h"
#include "chrome/browser/extensions/api/identity/web_auth_flow.h"
#include "url/gurl.h"

class Profile;

namespace extensions {

class BraveWebAuthFlow : public WebAuthFlow::Delegate {
 public:
  using CompleteFunctionWithErrorCallback =
      base::OnceCallback<void(const IdentityGetAuthTokenError& error)>;
  using CompleteFunctionWithResultCallback =
      base::OnceCallback<void(const std::string& access_token,
                              const std::set<std::string>& granted_scopes)>;

  BraveWebAuthFlow();
  ~BraveWebAuthFlow() override;

  static void SetTokenForTesting(const std::string& token);
  // Used only if Google API keys aren't set up.
  // WebAuthFlow::Delegate implementation:
  void OnAuthFlowFailure(WebAuthFlow::Failure failure) override;
  void OnAuthFlowURLChange(const GURL& redirect_url) override;

  // Used only if Google API keys aren't set up.
  void StartWebAuthFlow(
      Profile* profile,
      base::OnceClosure complete_mint_token_flow_callback,
      CompleteFunctionWithErrorCallback complete_with_error_callback,
      CompleteFunctionWithResultCallback complete_with_result_callback,
      const std::string& oauth2_client_id,
      ExtensionTokenKey token_key,
      bool interactive,
      bool user_gesture);

 private:
  static std::optional<std::string> token_for_testing_;

  raw_ptr<Profile> profile_;
  ExtensionTokenKey token_key_{/*extension_id=*/"",
                               /*account_info=*/CoreAccountInfo(),
                               /*scopes=*/{}};
  CompleteFunctionWithErrorCallback complete_with_error_callback_;
  CompleteFunctionWithResultCallback complete_with_result_callback_;
  // Used only if Google API keys aren't set up.
  std::unique_ptr<WebAuthFlow> web_auth_flow_;
  std::string redirect_scheme_;
  base::OnceClosure complete_mint_token_flow_callback_;
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_IDENTITY_BRAVE_WEB_AUTH_FLOW_H_
