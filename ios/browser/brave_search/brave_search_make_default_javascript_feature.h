// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SEARCH_BRAVE_SEARCH_MAKE_DEFAULT_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SEARCH_BRAVE_SEARCH_MAKE_DEFAULT_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>

#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class WebState;
}  // namespace web

class BraveSearchMakeDefaultJavaScriptFeature : public web::JavaScriptFeature {
 public:
  static BraveSearchMakeDefaultJavaScriptFeature* GetInstance();

  // JavaScriptFeature:
  bool GetFeatureRepliesToMessages() const override;
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  void ScriptMessageReceivedWithReply(
      web::WebState* web_state,
      const web::ScriptMessage& message,
      ScriptMessageReplyCallback callback) override;

 private:
  friend class base::NoDestructor<BraveSearchMakeDefaultJavaScriptFeature>;

  BraveSearchMakeDefaultJavaScriptFeature();
  ~BraveSearchMakeDefaultJavaScriptFeature() override;

  BraveSearchMakeDefaultJavaScriptFeature(
      const BraveSearchMakeDefaultJavaScriptFeature&) = delete;
  BraveSearchMakeDefaultJavaScriptFeature& operator=(
      const BraveSearchMakeDefaultJavaScriptFeature&) = delete;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_SEARCH_BRAVE_SEARCH_MAKE_DEFAULT_JAVASCRIPT_FEATURE_H_
