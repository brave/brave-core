/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/app/brave_core_main.h"

#import <XCTest/XCTest.h>

@interface BraveCoreMainTest : XCTestCase
@end

@implementation BraveCoreMainTest
- (void)testStartup {
  const auto e = [self expectationWithDescription:@""];
  const auto main =
      [[BraveCoreMain alloc] initWithUserAgent:@"test_user_agent"];
  [main scheduleLowPriorityStartupTasks];
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC),
                 dispatch_get_main_queue(), ^{
                   [e fulfill];
                 });
  [self waitForExpectations:@[ e ] timeout:5];
}
@end
