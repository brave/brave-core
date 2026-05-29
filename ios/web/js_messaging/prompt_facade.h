// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_JS_MESSAGING_PROMPT_FACADE_H_
#define BRAVE_IOS_WEB_JS_MESSAGING_PROMPT_FACADE_H_

#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"

class GURL;

namespace web {

class WebState;

// Routes synchronous `window.prompt()` payloads to JavaScriptFeatures that
// have opted in via `JavaScriptFeature::GetFeatureRepliesToPrompts()`. The
// prompt body is expected to be a JSON document with `handler` and `message`
// keys.
class PromptFacade {
 public:
  // Constructs PromptFacade. The calling code must retain ownership of
  // `web_state`, which cannot be null.
  explicit PromptFacade(web::WebState* web_state);

  // If `prompt` targets a registered JavaScriptFeature on the bound WebState,
  // dispatches the message and returns the JSON-encoded reply, otherwise
  // returns nullptr
  std::optional<std::string> HandleJavaScriptPrompt(GURL request_url,
                                                    bool is_main_frame,
                                                    const std::string& prompt);

 private:
  raw_ptr<WebState> web_state_;
};

}  // namespace web

#endif  // BRAVE_IOS_WEB_JS_MESSAGING_PROMPT_FACADE_H_
