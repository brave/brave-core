// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_CRW_WK_UI_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_CRW_WK_UI_HANDLER_H_

#include <ios/web/web_state/ui/crw_wk_ui_handler.h>  // IWYU pragma: export

#include <memory>

class GURL;

namespace web {

class WebState;

// Routes synchronous `window.prompt()` payloads to JavaScriptFeatures that
// have opted in via `JavaScriptFeature::GetFeatureRepliesToPrompts()`. The
// prompt body is expected to be the JSON document produced by
// `//brave/ios/web/js_messaging/resources/prompt_utils.ts`.
class JSPromptCommunicationHandler {
 public:
  virtual ~JSPromptCommunicationHandler() = default;

  // If `prompt` targets a registered JavaScriptFeature on the bound WebState,
  // dispatches the message and writes the JSON-encoded reply into `result`,
  // returning true. Otherwise returns false and `result` is left untouched.
  // A given prompt nonce may only be handled once; subsequent invocations
  // with the same nonce DCHECK and return true with an empty `result` so that
  // no native prompt UI is shown.
  virtual bool HandleJavaScriptPrompt(GURL request_url,
                                      bool is_main_frame,
                                      NSString* prompt,
                                      NSString** result) = 0;

  // Get a prompt handler for the given web state
  static JSPromptCommunicationHandler* GetJSPromptCommunicationHandler(
      WebState* web_state);
};

}  // namespace web

// A CRWWKUIHandler subclass which will override the WKUIDelegate methods to
// handle JavaScript prompts
@interface BraveCRWWKUIHandler : CRWWKUIHandler
@end

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEB_STATE_UI_CRW_WK_UI_HANDLER_H_
