/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_LOCAL_DATA_FILE_SERVICE_LOCAL_DATA_FILE_SERVICE_WRAPPER_H_
#define BRAVE_IOS_BROWSER_API_LOCAL_DATA_FILE_SERVICE_LOCAL_DATA_FILE_SERVICE_WRAPPER_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
/// The service that wraps brave core's `LocalDataFileService`
/// and provides access to useful APIs.
@interface LocalDataFileServiceWrapper : NSObject
/// Return a cleaned version of the URL for clean copy feature
- (NSURL*)cleanedURL:(NSURL*)url;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_LOCAL_DATA_FILE_SERVICE_LOCAL_DATA_FILE_SERVICE_WRAPPER_H_
