// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>

#include "base/no_destructor.h"
#include "brave/ios/web/js_messaging/message_handler_token.h"
#include "brave/ios/web/js_messaging/randomized_message_handler_name.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

// Drives the element picker UI on iOS.
//
// Unlike the other shields features this one injects no script at document
// load. The picker is only ever shown in response to an explicit user action,
// so `ShowPicker()` evaluates the bundle on demand, matching the desktop
// renderer's behaviour.
class ElementPickerJavaScriptFeature : public web::JavaScriptFeature {
 public:
  static ElementPickerJavaScriptFeature* GetInstance();

  // Injects the element picker into the main frame of `web_state`, or
  // un-minimizes it when it is already showing.
  void ShowPicker(web::WebState* web_state);

  // JavaScriptFeature:
  std::optional<std::string> GetScriptMessageHandlerName() const override;
  bool GetFeatureRepliesToPrompts() const override;
  bool GetFeatureRepliesToMessages() const override;
  void ScriptMessageReceivedWithReply(
      web::WebState* web_state,
      const web::ScriptMessage& message,
      ScriptMessageReplyCallback callback) override;

 private:
  friend class base::NoDestructor<ElementPickerJavaScriptFeature>;

  ElementPickerJavaScriptFeature();
  ~ElementPickerJavaScriptFeature() override;

  // Returns the placeholder replacements used to randomize the script's
  // message handler name and token at injection time.
  FeatureScript::PlaceholderReplacements GetReplacements();

  // The picker bundle. Held here rather than passed to the base class so that
  // it is not injected into every page load.
  FeatureScript picker_script_;

  web::RandomizedMessageHandlerName handler_name_;
  web::MessageHandlerToken token_;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_JAVASCRIPT_FEATURE_H_
