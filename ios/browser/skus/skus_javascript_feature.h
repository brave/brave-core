// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_SKUS_SKUS_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_SKUS_SKUS_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/supports_user_data.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace web {
class BrowserState;
class WebState;
}  // namespace web

class ProfileIOS;

namespace skus {

// A JavaScriptFeature that exposes a `window.chrome.braveSkus` API to the
// Brave accounts website (account.brave.com and its staging / dev
// counterparts) so the SKUs SDK on the page can ask the browser to refresh
// orders, fetch credentials, and prepare them for presentation. Calls are
// forwarded over mojo to a profile-scoped SkusService.
class SkusJavaScriptFeature : public web::JavaScriptFeature,
                              public base::SupportsUserData::Data {
 public:
  ~SkusJavaScriptFeature() override;

  using CredentialSummaryFetchedCallback =
      base::RepeatingCallback<void(const std::string domain,
                                   const std::string message)>;

  static SkusJavaScriptFeature* FromBrowserState(
      web::BrowserState* browser_state);

  void SetCredentialSummaryFetched(CredentialSummaryFetchedCallback callback);

  // web::JavaScriptFeature:
  bool GetFeatureRepliesToMessages() const override;
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  std::vector<web::JavaScriptFeature::FeatureScript> GetScripts()
      const override;
  void ScriptMessageReceivedWithReply(
      web::WebState* web_state,
      const web::ScriptMessage& message,
      ScriptMessageReplyCallback callback) override;

 private:
  explicit SkusJavaScriptFeature(ProfileIOS* profile);

  // Lazily binds `skus_service_` to a fresh remote for `profile_`. After this
  // call `skus_service_` is bound iff the SKUs service is available for the
  // profile.
  void EnsureMojoConnected();
  void OnMojoConnectionError();
  void OnCredentialSummary(const std::string domain,
                           ScriptMessageReplyCallback callback,
                           mojom::SkusResultPtr response);

  CredentialSummaryFetchedCallback credential_summary_fetched_;
  raw_ptr<ProfileIOS> profile_;
  mojo::Remote<mojom::SkusService> skus_service_;
};

}  // namespace skus

#endif  // BRAVE_IOS_BROWSER_SKUS_SKUS_JAVASCRIPT_FEATURE_H_
