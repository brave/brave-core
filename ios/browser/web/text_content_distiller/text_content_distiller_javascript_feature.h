// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_TEXT_CONTENT_DISTILLER_TEXT_CONTENT_DISTILLER_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_WEB_TEXT_CONTENT_DISTILLER_TEXT_CONTENT_DISTILLER_JAVASCRIPT_FEATURE_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class WebState;
}  // namespace web

// Distills a page's main text content, shared by AI Chat (article
// summarization) and ads (text classification).
class TextContentDistillerJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static TextContentDistillerJavaScriptFeature* GetInstance();

  TextContentDistillerJavaScriptFeature(
      const TextContentDistillerJavaScriptFeature&) = delete;
  TextContentDistillerJavaScriptFeature& operator=(
      const TextContentDistillerJavaScriptFeature&) = delete;

  // Fetches the distilled text content from `web_state` and returns it as a
  // string via `callback`. Returns an empty string if the content cannot be
  // retrieved.
  void GetTextContent(web::WebState* web_state,
                      base::OnceCallback<void(std::string)> callback);

 private:
  friend class base::NoDestructor<TextContentDistillerJavaScriptFeature>;

  TextContentDistillerJavaScriptFeature();
  ~TextContentDistillerJavaScriptFeature() override;
};

#endif  // BRAVE_IOS_BROWSER_WEB_TEXT_CONTENT_DISTILLER_TEXT_CONTENT_DISTILLER_JAVASCRIPT_FEATURE_H_
