/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_INTERNALS_H_
#define BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_INTERNALS_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <Webkit/Webkit.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveSyncInternalsController : UIViewController
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_INTERNALS_H_
