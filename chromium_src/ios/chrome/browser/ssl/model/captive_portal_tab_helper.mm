// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ios/chrome/browser/web/model/web_navigation_util.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"

#include <ios/chrome/browser/ssl/model/captive_portal_tab_helper.mm>

BraveCaptivePortalTabHelper::BraveCaptivePortalTabHelper(
    web::WebState* web_state)
    : web_state_(web_state) {
  web_state_->AddObserver(this);
}

BraveCaptivePortalTabHelper::~BraveCaptivePortalTabHelper() {
  if (web_state_) {
    web_state_->RemoveObserver(this);
  }
}

// Displays the Captive Portal Login page at `landing_url`.
void BraveCaptivePortalTabHelper::DisplayCaptivePortalLoginPage(
    GURL landing_url) {
  if (!web_state_) {
    return;
  }
  web_state_->GetNavigationManager()->LoadURLWithParams(
      web_navigation_util::CreateWebLoadParams(
          landing_url, ui::PAGE_TRANSITION_TYPED, nullptr));
}

// WebObserver
void BraveCaptivePortalTabHelper::WebStateDestroyed(web::WebState* web_state) {
  DCHECK_EQ(web_state_, web_state);
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}
