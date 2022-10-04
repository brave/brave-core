/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_P3A_BRAVE_P3A_UTILS_H_
#define BRAVE_IOS_BROWSER_API_P3A_BRAVE_P3A_UTILS_H_

#import <Foundation/Foundation.h>

@class BraveHistogramsController;

typedef NSString* HistogramKeyName;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveP3AUtils : NSObject
@property(nonatomic) bool isP3AEnabled;
@property(nonatomic) bool isNoticeAcknowledged;
- (BraveHistogramsController*)histogramsController;
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_P3A_BRAVE_P3A_UTILS_H_
