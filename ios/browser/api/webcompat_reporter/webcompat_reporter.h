// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_H_
#define BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_H_

#import <Foundation/Foundation.h>
#import "webcompat_reporter.mojom.objc.h"

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface WebcompatReporterAPI : NSObject
- (void)submitReport;//:(WebcompatReporterReportInfo*)reportInfo;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_H_
