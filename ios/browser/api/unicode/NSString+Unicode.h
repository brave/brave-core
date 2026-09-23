// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_UNICODE_NSSTRING_UNICODE_H_
#define BRAVE_IOS_BROWSER_API_UNICODE_NSSTRING_UNICODE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, ICUUBiDiDirection) {
  ICUUBiDiDirectionLeftToRight,
  ICUUBiDiDirectionRightToLeft,
  ICUUBiDiDirectionMixed,
  ICUUBiDiDirectionNeutral
};

OBJC_EXPORT
@interface NSString (Unicode)
/// Determines the *overall* direction of the text after processing its entire
/// contents.
/// - returns: `Left-To-Right`, `Right-To-Left`, `Mixed`
/// - note: Can never return `Neutral`
@property(nonatomic, readonly) ICUUBiDiDirection bidiDirection;

/// Determines the *base* directionality of the text,
/// based only on the *first* strong directional character, or the absence of
/// one.
/// - returns: `Left-To-Right`, `Right-To-Left`, `Neutral`
/// - note: Can never return `Mixed`
@property(nonatomic, readonly) ICUUBiDiDirection bidiBaseDirection;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_UNICODE_NSSTRING_UNICODE_H_
