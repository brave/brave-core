/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_AUTOFILL_BRAVE_WEB_VIEW_AUTOFILL_LOOKUP_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_AUTOFILL_BRAVE_WEB_VIEW_AUTOFILL_LOOKUP_H_

#import <Foundation/Foundation.h>

namespace web {
class WebState;
}

@class CWVAutofillController;

// Implemented in brave_web_view_autofill_lookup.mm (//brave/.../web_view target).
// Returns the tab's `autofillController` when `WebState` is backed by a
// BraveWebView; nil otherwise.
CWVAutofillController* BraveWebViewAutofillControllerForWebState(
    web::WebState* web_state);

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_AUTOFILL_BRAVE_WEB_VIEW_AUTOFILL_LOOKUP_H_
