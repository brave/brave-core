// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web/js_messaging/protected_javascript_feature.h"

#include "ios/web/public/js_messaging/script_message.h"

namespace web {

ProtectedJavaScriptFeature::~ProtectedJavaScriptFeature() = default;

std::optional<std::string>
ProtectedJavaScriptFeature::GetScriptMessageHandlerName() const {
  return handler_name_.GetScriptMessageHandlerName();
}

std::optional<const base::Value*>
ProtectedJavaScriptFeature::GetValidatedScriptMessageBody(
    const web::ScriptMessage& script_message) {
  return token_.GetValidatedScriptMessageBody(script_message);
}

JavaScriptFeature::FeatureScript::PlaceholderReplacements
ProtectedJavaScriptFeature::GetPlaceholderReplacements() const {
  NSMutableDictionary* replacements = [[NSMutableDictionary alloc] init];
  [replacements addEntriesFromDictionary:token_.GetPlaceholderReplacements()];
  [replacements
      addEntriesFromDictionary:handler_name_.GetPlaceholderReplacements()];
  return [replacements copy];
}

}  // namespace web
