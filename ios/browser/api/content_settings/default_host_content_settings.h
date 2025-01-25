// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_CONTENT_SETTINGS_DEFAULT_HOST_CONTENT_SETTINGS_H_
#define BRAVE_IOS_BROWSER_API_CONTENT_SETTINGS_DEFAULT_HOST_CONTENT_SETTINGS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// The mode in which pages should be loaded.
typedef NS_ENUM(NSUInteger, DefaultPageMode) {
  DefaultPageModeMobile,
  DefaultPageModeDesktop,
};

/// A list of default website preferences.
OBJC_EXPORT
@interface DefaultHostContentSettings : NSObject

/// The default page mode in which pages should be loaded.
@property(nonatomic) DefaultPageMode defaultPageMode;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_CONTENT_SETTINGS_DEFAULT_HOST_CONTENT_SETTINGS_H_
