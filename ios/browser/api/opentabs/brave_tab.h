/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_TAB_H_
#define BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_TAB_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveTab: NSObject
- (void)setTitle:(NSString*)title;
- (void)setURL:(NSURL*)url;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_OPENTABS_BRAVE_TAB_H_
