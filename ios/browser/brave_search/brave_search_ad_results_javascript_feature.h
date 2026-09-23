// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SEARCH_BRAVE_SEARCH_AD_RESULTS_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SEARCH_BRAVE_SEARCH_AD_RESULTS_JAVASCRIPT_FEATURE_H_

#include "base/functional/callback_forward.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

namespace web {
class WebState;
}  // namespace web

class BraveSearchAdResultsJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static BraveSearchAdResultsJavaScriptFeature* GetInstance();

  // Fetches search result ad creatives from `web_state` and returns them as a
  // JSON string via `callback`. The callback receives nullptr if the creatives
  // cannot be retrieved.
  void GetCreatives(web::WebState* web_state,
                    base::OnceCallback<void(const base::Value*)> callback);

 private:
  friend class base::NoDestructor<BraveSearchAdResultsJavaScriptFeature>;

  BraveSearchAdResultsJavaScriptFeature();
  ~BraveSearchAdResultsJavaScriptFeature() override;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_SEARCH_BRAVE_SEARCH_AD_RESULTS_JAVASCRIPT_FEATURE_H_
