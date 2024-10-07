// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_PRIVATE_H_

#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter.h"

class ChromeBrowserState;

@interface WebcompatReporter (Private)
- (instancetype)initWithChromeBrowserState:(ChromeBrowserState*)browserState;
@end

#endif  // BRAVE_IOS_BROWSER_API_WEBCOMPAT_REPORTER_WEBCOMPAT_REPORTER_PRIVATE_H_