// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_BRAVE_SHIELDS_REQUEST_BLOCKING_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_WEB_BRAVE_SHIELDS_REQUEST_BLOCKING_JAVASCRIPT_FEATURE_H_

#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

class RequestBlockingJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static RequestBlockingJavaScriptFeature* GetInstance();

  // JavaScriptFeature:
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  bool GetFeatureRepliesToMessages() const override;
  void ScriptMessageReceivedWithReply(
      web::WebState* web_state,
      const web::ScriptMessage& message,
      ScriptMessageReplyCallback callback) override;

 private:
  friend class base::NoDestructor<RequestBlockingJavaScriptFeature>;

  RequestBlockingJavaScriptFeature();
  ~RequestBlockingJavaScriptFeature() override;
};

#endif  // BRAVE_IOS_BROWSER_WEB_BRAVE_SHIELDS_REQUEST_BLOCKING_JAVASCRIPT_FEATURE_H_
