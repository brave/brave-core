/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <Cocoa/Cocoa.h>

#import "brave/browser/brave_app_controller_mac.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/platform_test.h"

class BraveAppControllerTest : public PlatformTest {
 protected:
  BraveAppControllerTest() {}

  void SetUp() override {
    PlatformTest::SetUp();
    braveAppController_.reset([[BraveAppController alloc] init]);
    copyMenuItem_.reset([[NSMenuItem alloc] initWithTitle:@""
                                                   action:0
                                            keyEquivalent:@""]);
    [braveAppController_ setCopyMenuItemForTesting:copyMenuItem_];

    copyCleanLinkMenuItem_.reset([[NSMenuItem alloc] initWithTitle:@""
                                                            action:0
                                                     keyEquivalent:@""]);
    [braveAppController_
        setCopyCleanLinkMenuItemForTesting:copyCleanLinkMenuItem_];
  }

  void TearDown() override {
    [braveAppController_ setCopyMenuItemForTesting:nil];
    [braveAppController_ setCopyCleanLinkMenuItemForTesting:nil];
    PlatformTest::TearDown();
  }

  base::scoped_nsobject<BraveAppController> braveAppController_;
  base::scoped_nsobject<NSMenuItem> copyMenuItem_;
  base::scoped_nsobject<NSMenuItem> copyCleanLinkMenuItem_;
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(BraveAppControllerTest, OnlyCopyItem) {
  [braveAppController_ setSelectedURLForTesting:false];

  [braveAppController_ menuNeedsUpdate:[copyMenuItem_ menu]];

  EXPECT_TRUE([copyCleanLinkMenuItem_ isHidden]);
}

TEST_F(BraveAppControllerTest, CleanLinkItemAdded) {
  [braveAppController_ setSelectedURLForTesting:true];

  [braveAppController_ menuNeedsUpdate:[copyMenuItem_ menu]];

  EXPECT_FALSE([copyCleanLinkMenuItem_ isHidden]);
}
