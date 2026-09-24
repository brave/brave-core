// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SSL_MODEL_CAPTIVE_PORTAL_TAB_HELPER_H_
#define BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SSL_MODEL_CAPTIVE_PORTAL_TAB_HELPER_H_

#include <ios/chrome/browser/ssl/model/captive_portal_tab_helper.h>  // IWYU pragma: export

#include "ios/web/public/web_state_observer.h"

// A custom captive portal tab helper that instead just loads the landing URL
// directly into the current WebState instead of attempting to open it in a new
// tab since we dont use WebStateList & its insertion agent on iOS
class BraveCaptivePortalTabHelper
    : public web::WebStateUserData<BraveCaptivePortalTabHelper>,
      web::WebStateObserver {
 public:
  BraveCaptivePortalTabHelper(const BraveCaptivePortalTabHelper&) = delete;
  BraveCaptivePortalTabHelper& operator=(const BraveCaptivePortalTabHelper&) =
      delete;

  ~BraveCaptivePortalTabHelper() override;

  // Displays the Captive Portal Login page at `landing_url`.
  void DisplayCaptivePortalLoginPage(GURL landing_url);

  // WebObserver
  void WebStateDestroyed(web::WebState* web_state) override;

 private:
  friend class web::WebStateUserData<BraveCaptivePortalTabHelper>;
  explicit BraveCaptivePortalTabHelper(web::WebState* web_state);
  raw_ptr<web::WebState> web_state_ = nullptr;
};

#endif  // BRAVE_CHROMIUM_SRC_IOS_CHROME_BROWSER_SSL_MODEL_CAPTIVE_PORTAL_TAB_HELPER_H_
