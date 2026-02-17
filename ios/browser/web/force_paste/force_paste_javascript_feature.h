// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_FORCE_PASTE_FORCE_PASTE_JAVASCRIPT_FEATURE_H_
#define BRAVE_IOS_BROWSER_WEB_FORCE_PASTE_FORCE_PASTE_JAVASCRIPT_FEATURE_H_

#include "base/no_destructor.h"
#include "ios/web/public/js_messaging/java_script_feature.h"

class ForcePasteJavaScriptFeature : public web::JavaScriptFeature {
 public:
  // This feature holds no state, so only a single static instance is ever
  // needed.
  static ForcePasteJavaScriptFeature* GetInstance();

  // Force pastes the contents into the active element on the web_state
  void ForcePaste(web::WebState* web_state, std::string contents);

 private:
  friend class base::NoDestructor<ForcePasteJavaScriptFeature>;

  ForcePasteJavaScriptFeature();
  ~ForcePasteJavaScriptFeature() override;
};

#endif  // BRAVE_IOS_BROWSER_WEB_FORCE_PASTE_FORCE_PASTE_JAVASCRIPT_FEATURE_H_
