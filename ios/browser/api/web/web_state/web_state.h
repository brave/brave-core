/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_WEB_WEB_STATE_WEB_STATE_H_
#define BRAVE_IOS_BROWSER_API_WEB_WEB_STATE_WEB_STATE_H_

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(WebState)
OBJC_EXPORT
@interface WebState : NSObject
- (instancetype)init NS_UNAVAILABLE;

//- (void)setTitle:(NSString*)title;
//- (void)setURL:(NSURL*)url;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEB_WEB_STATE_WEB_STATE_H_
