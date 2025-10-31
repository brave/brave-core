// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_INTERNAL_H_
#define BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_INTERNAL_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/api/web_view/brave_web_view.h"

@class CWVWebView;

namespace web {
class WebState;
}

NS_ASSUME_NONNULL_BEGIN

@interface BraveWebView (Internal)
/// A method to call when a download task is created to ensure properties are
/// updated
- (void)updateForOnDownloadCreated;
/// The original method marked as unavailable because it is unsafe as it doesn't
/// ensure the WebViewHolder exists in the WebState
+ (CWVWebView*)webViewForWebState:(web::WebState*)webState NS_UNAVAILABLE;
/// A safe method of obtaining a BraveWebView from a WebState
+ (nullable BraveWebView*)braveWebViewForWebState:(web::WebState*)webState;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_VIEW_BRAVE_WEB_VIEW_INTERNAL_H_
