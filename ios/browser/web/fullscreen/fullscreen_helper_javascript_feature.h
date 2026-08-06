// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_FULLSCREEN_FULLSCREEN_HELPER_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_WEB_FULLSCREEN_FULLSCREEN_HELPER_JAVASCRIPT_FEATURE_H_

#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

class FullscreenHelperJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static FullscreenHelperJavaScriptFeature* GetInstance();

 private:
  friend class base::NoDestructor<FullscreenHelperJavaScriptFeature>;

  FullscreenHelperJavaScriptFeature();
  ~FullscreenHelperJavaScriptFeature() override;
};

#endif  // BRAVE_IOS_BROWSER_WEB_FULLSCREEN_FULLSCREEN_HELPER_JAVASCRIPT_FEATURE_H_
