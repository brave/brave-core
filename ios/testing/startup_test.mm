/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <XCTest/XCTest.h>
#import "brave/ios/app/brave_core_main.h"

@interface BraveCoreMainStartupTest : XCTestCase
@end

@implementation BraveCoreMainStartupTest

- (void)testBraveCoreMainStartup {
  auto core = [[BraveCoreMain alloc] initWithUserAgent:@"UnitTest"];
  [core scheduleLowPriorityStartupTasks];
}

@end
