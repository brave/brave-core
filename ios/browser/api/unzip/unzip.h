// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_UNZIP_UNZIP_H_
#define BRAVE_IOS_BROWSER_API_UNZIP_UNZIP_H_

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(Unzip)
@interface BraveUnzip : NSObject
+ (void)unzip:(NSString*)zipFile
    toDirectory:(NSString*)directory
     completion:(void (^)(bool))completion;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_UNZIP_UNZIP_H_
