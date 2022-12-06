/* Copyright (c) 2023 The Brave Authors. All rights reserved.
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

  void CheckHotkeysOnCopyItem() {
    [braveAppController_ setSelectedURLForTesting:false];

    [braveAppController_ menuNeedsUpdate:[copyMenuItem_ menu]];

    EXPECT_TRUE([[copyMenuItem_ keyEquivalent] isEqualToString:@"c"]);
    EXPECT_EQ([copyMenuItem_ keyEquivalentModifierMask],
              NSEventModifierFlagCommand);

    EXPECT_TRUE([[copyCleanLinkMenuItem_ keyEquivalent] isEqualToString:@""]);
    EXPECT_EQ([copyCleanLinkMenuItem_ keyEquivalentModifierMask], 0UL);
    EXPECT_TRUE([copyCleanLinkMenuItem_ isHidden]);
  }

  void CheckHotkeysOnCleanLinkItem() {
    [braveAppController_ setSelectedURLForTesting:true];

    [braveAppController_ menuNeedsUpdate:[copyMenuItem_ menu]];

    EXPECT_TRUE([[copyMenuItem_ keyEquivalent] isEqualToString:@""]);
    EXPECT_EQ([copyMenuItem_ keyEquivalentModifierMask], 0UL);

    EXPECT_TRUE([[copyCleanLinkMenuItem_ keyEquivalent] isEqualToString:@"c"]);
    EXPECT_EQ([copyCleanLinkMenuItem_ keyEquivalentModifierMask],
              NSEventModifierFlagCommand);
    EXPECT_FALSE([copyCleanLinkMenuItem_ isHidden]);
  }

  base::scoped_nsobject<BraveAppController> braveAppController_;
  base::scoped_nsobject<NSMenuItem> copyMenuItem_;
  base::scoped_nsobject<NSMenuItem> copyCleanLinkMenuItem_;
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(BraveAppControllerTest, OnlyCopyItem) {
  CheckHotkeysOnCopyItem();
}

TEST_F(BraveAppControllerTest, CleanLinkItemAdded) {
  CheckHotkeysOnCleanLinkItem();
}
