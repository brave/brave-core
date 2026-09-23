// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_NIGHT_MODE_NIGHT_MODE_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_WEB_NIGHT_MODE_NIGHT_MODE_JAVASCRIPT_FEATURE_H_

#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

class NightModeJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static NightModeJavaScriptFeature* GetInstance();

 private:
  friend class base::NoDestructor<NightModeJavaScriptFeature>;

  NightModeJavaScriptFeature();
  ~NightModeJavaScriptFeature() override;
};

#endif  // BRAVE_IOS_BROWSER_WEB_NIGHT_MODE_NIGHT_MODE_JAVASCRIPT_FEATURE_H_
