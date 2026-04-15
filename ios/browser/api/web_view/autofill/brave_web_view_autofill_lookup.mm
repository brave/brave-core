/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/web_view/autofill/brave_web_view_autofill_lookup.h"

#import "brave/ios/browser/api/web_view/brave_web_view_internal.h"
#import "ios/web_view/public/cwv_autofill_controller.h"

CWVAutofillController* BraveWebViewAutofillControllerForWebState(
    web::WebState* web_state) {
  BraveWebView* web_view = [BraveWebView braveWebViewForWebState:web_state];
  return web_view.autofillController;
}
