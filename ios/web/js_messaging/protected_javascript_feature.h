// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_JS_MESSAGING_PAGED_JAVASCRIPT_FEATURE_UTILS_H_
#define BRAVE_IOS_WEB_JS_MESSAGING_PAGED_JAVASCRIPT_FEATURE_UTILS_H_

#include <optional>
#include <string>

#include "base/uuid.h"
#include "ios/web/public/js_messaging/java_script_feature.h"
namespace base {
class Value;
}

/// A JavaScriptFeature that will apply safe built-ins and provide methods for
/// validating message bodies with a security token and randomized message
/// handler names
class ProtectedJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // web::JavaScriptFeature
  std::optional<std::string> GetScriptMessageHandlerName() const override;

 protected:
  base::Uuid GetSecurityToken();
  std::string_view GetMessageHandlerName();
  std::optional<base::Value> ValidatedBodyFromScriptMessage(
      const web::ScriptMessage& message);

 private:
  base::Uuid security_token_;
  std::string message_handler_name_;
};

#endif  // BRAVE_IOS_WEB_JS_MESSAGING_PAGED_JAVASCRIPT_FEATURE_UTILS_H_
