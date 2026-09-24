// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/js_messaging/randomized_message_handler_name.h"

#include "base/strings/sys_string_conversions.h"

namespace web {

std::string RandomizedMessageHandlerName::GetScriptMessageHandlerName() const {
  return handler_name_.AsLowercaseString();
}

web::JavaScriptFeature::FeatureScript::PlaceholderReplacements
RandomizedMessageHandlerName::GetPlaceholderReplacements() const {
  return @{
    @"gCrWebPlaceholderMessageHandlerName" :
        base::SysUTF8ToNSString(GetScriptMessageHandlerName())
  };
}

}  // namespace web
