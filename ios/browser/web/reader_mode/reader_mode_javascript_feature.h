// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_READER_MODE_READER_MODE_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_WEB_READER_MODE_READER_MODE_JAVASCRIPT_FEATURE_H_

#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace brave {

// A JavaScriptFeature that injects the Readability-based reader mode script
// into web pages. It checks whether pages are readable, parses them, and
// manages the reader mode display style.
class ReaderModeJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static ReaderModeJavaScriptFeature* GetInstance();

  // Checks whether the current page is readable and returns the parsed result
  // as a JSON string via |callback|. An empty string is passed if the page is
  // not readable or the call times out.
  void CheckReadability(web::WebState* web_state,
                        base::OnceCallback<void(const std::string&)> callback);

  // Updates the reader mode display style on the main frame of |web_state|.
  // |style_dict| must contain "theme", "fontType", and "fontSize" keys.
  void SetStyle(web::WebState* web_state, const base::DictValue& style_dict);

 private:
  friend class base::NoDestructor<ReaderModeJavaScriptFeature>;

  ReaderModeJavaScriptFeature();
  ~ReaderModeJavaScriptFeature() override;

  ReaderModeJavaScriptFeature(const ReaderModeJavaScriptFeature&) = delete;
  ReaderModeJavaScriptFeature& operator=(const ReaderModeJavaScriptFeature&) =
      delete;
};

}  // namespace brave

#endif  // BRAVE_IOS_BROWSER_WEB_READER_MODE_READER_MODE_JAVASCRIPT_FEATURE_H_
