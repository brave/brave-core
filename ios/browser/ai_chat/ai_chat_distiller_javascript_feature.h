// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_DISTILLER_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_DISTILLER_JAVASCRIPT_FEATURE_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class WebState;
}  // namespace web

class AIChatDistillerJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static AIChatDistillerJavaScriptFeature* GetInstance();

  // Fetches the main article text content from `web_state` and returns it
  // as a string via `callback`. Returns an empty string if the content
  // cannot be retrieved.
  void GetMainArticle(web::WebState* web_state,
                      base::OnceCallback<void(std::string)> callback);

 private:
  friend class base::NoDestructor<AIChatDistillerJavaScriptFeature>;

  AIChatDistillerJavaScriptFeature();
  ~AIChatDistillerJavaScriptFeature() override;
};

#endif  // BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_DISTILLER_JAVASCRIPT_FEATURE_H_
