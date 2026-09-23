// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_TALK_BRAVE_TALK_LAUNCHER_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_BRAVE_TALK_BRAVE_TALK_LAUNCHER_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>

#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class WebState;
}  // namespace web

// A JavaScriptFeature that observes iframe additions on Brave Talk pages and
// notifies the BraveTalkTabHelper attached to the WebState to launch the
// native Brave Talk experience.
class BraveTalkLauncherJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static BraveTalkLauncherJavaScriptFeature* GetInstance();

  // JavaScriptFeature:
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  void ScriptMessageReceived(web::WebState* web_state,
                             const web::ScriptMessage& message) override;

 private:
  friend class base::NoDestructor<BraveTalkLauncherJavaScriptFeature>;

  BraveTalkLauncherJavaScriptFeature();
  ~BraveTalkLauncherJavaScriptFeature() override;

  BraveTalkLauncherJavaScriptFeature(
      const BraveTalkLauncherJavaScriptFeature&) = delete;
  BraveTalkLauncherJavaScriptFeature& operator=(
      const BraveTalkLauncherJavaScriptFeature&) = delete;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_TALK_BRAVE_TALK_LAUNCHER_JAVASCRIPT_FEATURE_H_
