/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_EXTRAS_H_
#define BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_EXTRAS_H_

#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>

#import "cwv_export.h"
#import "cwv_web_view.h"

NS_ASSUME_NONNULL_BEGIN

CWV_EXPORT
@interface CWVWebView (Extras)
- (void)updateScripts;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_WEB_VIEW_PUBLIC_CWV_WEB_VIEW_EXTRAS_H_
