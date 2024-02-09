// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_DEBOUNCE_DEBOUNCE_SERVICE_H_
#define BRAVE_IOS_BROWSER_API_DEBOUNCE_DEBOUNCE_SERVICE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface DebounceService : NSObject
- (instancetype)init NS_UNAVAILABLE;

/**
 True or false if debounce is enabled
 */
@property(nonatomic) bool isEnabled;

/**
 Debounces the given URL.

 @param url The URL to be debounced.
 @return A debounced NSURL object.
 */
- (nullable NSURL*)debounceURL:(NSURL*)url;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_DEBOUNCE_DEBOUNCE_SERVICE_H_
