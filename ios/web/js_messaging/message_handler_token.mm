// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/js_messaging/message_handler_token.h"

#include <optional>

#include "base/strings/sys_string_conversions.h"
#include "ios/web/public/js_messaging/script_message.h"

namespace web {

namespace {
constexpr char kTokenKey[] = "token";
constexpr char kMessageKey[] = "message";
}  // namespace

std::optional<const base::Value*>
MessageHandlerToken::GetValidatedScriptMessageBody(
    const web::ScriptMessage& script_message) {
  const base::DictValue* script_dict =
      script_message.legacy_body() ? script_message.legacy_body()->GetIfDict()
                                   : nullptr;
  if (!script_dict) {
    return std::nullopt;
  }

  const std::string* token = script_dict->FindString(kTokenKey);
  if (!token || *token != token_.AsLowercaseString()) {
    return std::nullopt;
  }

  const base::Value* message = script_dict->Find(kMessageKey);
  if (!message) {
    return std::nullopt;
  }

  return message;
}

web::JavaScriptFeature::FeatureScript::PlaceholderReplacements
MessageHandlerToken::GetPlaceholderReplacements() const {
  return @{
    @"gCrWebPlaceholderMessageHandlerToken" :
        base::SysUTF8ToNSString(token_.AsLowercaseString())
  };
}

}  // namespace web
