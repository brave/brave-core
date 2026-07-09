// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_SOLANA_PROVIDER_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_SOLANA_PROVIDER_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/supports_user_data.h"
#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "components/prefs/pref_change_registrar.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class BrowserState;
class ScriptMessage;
class WebState;
}  // namespace web

class ProfileIOS;

namespace brave_wallet {

// A JavaScriptFeature that injects the `window.solana`/`window.braveSolana`
// provider into web pages so they can communicate with Brave Wallet. Alongside
// the main provider script this also injects the `@solana/web3.js` module and
// the Wallet Standard library that the provider depends on. The provider is
// only installed when Brave Wallet is allowed for the profile and Brave is the
// default Solana wallet.
class SolanaProviderJavaScriptFeature : public web::JavaScriptFeature,
                                        public base::SupportsUserData::Data {
 public:
  ~SolanaProviderJavaScriptFeature() override;

  static SolanaProviderJavaScriptFeature* FromBrowserState(
      web::BrowserState* browser_state);

  // web::JavaScriptFeature:
  std::vector<web::JavaScriptFeature::FeatureScript> GetScripts()
      const override;
  bool GetFeatureRepliesToMessages() const override;
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  void ScriptMessageReceivedWithReply(
      web::WebState* web_state,
      const web::ScriptMessage& message,
      ScriptMessageReplyCallback callback) override;

 private:
  explicit SolanaProviderJavaScriptFeature(ProfileIOS* profile);

  void OnDefaultSolanaWalletChanged();

  web::MessageHandlerToken token_;
  raw_ptr<ProfileIOS> profile_;
  std::string provider_bundle_js_;
  std::string solana_web3_js_;
  std::string wallet_standard_js_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_SOLANA_PROVIDER_JAVASCRIPT_FEATURE_H_
