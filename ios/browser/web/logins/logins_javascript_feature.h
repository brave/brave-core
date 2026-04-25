// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_LOGINS_LOGINS_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_WEB_LOGINS_LOGINS_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

// JavaScriptFeature that injects the logins script into all frames and
// handles the bidirectional messaging for login autofill and credential
// saving. Replaces the WKUserScript + LoginsScriptHandler architecture with
// the Chromium iOS JavaScriptFeature messaging infrastructure.
class LoginsJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature is a singleton; the delegate carries per-browser state.
  static LoginsJavaScriptFeature* GetInstance();

  // JavaScriptFeature:
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  bool GetFeatureRepliesToMessages() const override;
  void ScriptMessageReceivedWithReply(
      web::WebState* web_state,
      const web::ScriptMessage& message,
      ScriptMessageReplyCallback callback) override;

 private:
  friend class base::NoDestructor<LoginsJavaScriptFeature>;

  LoginsJavaScriptFeature();
  ~LoginsJavaScriptFeature() override;
};

#endif  // BRAVE_IOS_BROWSER_WEB_LOGINS_LOGINS_JAVASCRIPT_FEATURE_H_
