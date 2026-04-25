// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_BROWSING_DATA_UTILS_H_
#define BRAVE_IOS_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_BROWSING_DATA_UTILS_H_

#import <Foundation/Foundation.h>

@protocol ProfileBridge;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT void BraveRemoveSiteSettingsData(NSDate* delete_begin,
                                             NSDate* delete_end,
                                             id<ProfileBridge> profile);

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_BROWSING_DATA_UTILS_H_
