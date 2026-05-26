// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_USER_SCRIPTS_USER_SCRIPTS_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_WEB_USER_SCRIPTS_USER_SCRIPTS_JAVASCRIPT_FEATURE_H_

#include "base/no_destructor.h"
#include "brave/ios/web/js_messaging/protected_javascript_feature.h"

class UserScriptsJavaScriptFeature : public web::ProtectedJavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static UserScriptsJavaScriptFeature* GetInstance();

  void ScriptMessageReceived(web::WebState* web_state,
                             const web::ScriptMessage& message) override;

 private:
  friend class base::NoDestructor<UserScriptsJavaScriptFeature>;

  UserScriptsJavaScriptFeature();
  ~UserScriptsJavaScriptFeature() override;
};

#endif  // BRAVE_IOS_BROWSER_WEB_USER_SCRIPTS_USER_SCRIPTS_JAVASCRIPT_FEATURE_H_
