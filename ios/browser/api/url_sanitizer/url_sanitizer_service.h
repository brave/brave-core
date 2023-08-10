// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_URL_SANITIZER_URL_SANITIZER_SERVICE_H_
#define BRAVE_IOS_BROWSER_API_URL_SANITIZER_URL_SANITIZER_SERVICE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface URLSanitizerService : NSObject
- (instancetype)init NS_UNAVAILABLE;

/**
 Sanitizes the given URL.

 @param url The URL to be sanitized.
 @return A sanitized NSURL object.
 */
- (nullable NSURL*)sanitizedURL:(NSURL*)url;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_URL_SANITIZER_URL_SANITIZER_SERVICE_H_
