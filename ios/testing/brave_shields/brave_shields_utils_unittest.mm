// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

#include "base/test/task_environment.h"
#include "brave/ios/browser/api/brave_shields/brave_shields_utils_ios+private.h"
#import "ios/chrome/browser/shared/model/profile/test/test_profile_ios.h"
#import "testing/gtest/include/gtest/gtest.h"
#import "testing/platform_test.h"

class BraveShieldsUtilsTest : public PlatformTest {
 public:
  explicit BraveShieldsUtilsTest() {
    TestProfileIOS::Builder builder;
    profile_ = std::move(builder).Build();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestProfileIOS> profile_;
};

TEST_F(BraveShieldsUtilsTest, TestBraveShieldsEnabled) {
  auto* braveShieldsUtils =
      [[BraveShieldsUtilsIOS alloc] initWithBrowserState:profile_.get()];
  NSURL* testURL = [NSURL URLWithString:@"https://www.example.com"];
  EXPECT_TRUE([braveShieldsUtils braveShieldsEnabledFor:testURL
                                              isPrivate:false]);
  [braveShieldsUtils setBraveShieldsEnabled:false
                                     forURL:testURL
                                  isPrivate:false];
  EXPECT_FALSE([braveShieldsUtils braveShieldsEnabledFor:testURL
                                               isPrivate:false]);
}

TEST_F(BraveShieldsUtilsTest, TestAdBlockMode) {
  auto* braveShieldsUtils =
      [[BraveShieldsUtilsIOS alloc] initWithBrowserState:profile_.get()];
  NSURL* testURL = [NSURL URLWithString:@"https://www.example.com"];
  EXPECT_EQ([braveShieldsUtils adBlockModeForURL:testURL isPrivate:false],
            BraveShieldsAdBlockModeStandard);
  [braveShieldsUtils setAdBlockMode:BraveShieldsAdBlockModeAggressive
                             forURL:testURL
                          isPrivate:false];
  EXPECT_EQ([braveShieldsUtils adBlockModeForURL:testURL isPrivate:false],
            BraveShieldsAdBlockModeAggressive);
  [braveShieldsUtils setAdBlockMode:BraveShieldsAdBlockModeAllow
                             forURL:testURL
                          isPrivate:false];
  EXPECT_EQ([braveShieldsUtils adBlockModeForURL:testURL isPrivate:false],
            BraveShieldsAdBlockModeAllow);
}

TEST_F(BraveShieldsUtilsTest, TestBlockScripts) {
  auto* braveShieldsUtils =
      [[BraveShieldsUtilsIOS alloc] initWithBrowserState:profile_.get()];
  NSURL* testURL = [NSURL URLWithString:@"https://www.example.com"];
  EXPECT_FALSE([braveShieldsUtils blockScriptsEnabledForURL:testURL
                                                  isPrivate:false]);
  [braveShieldsUtils setBlockScriptsEnabled:true
                                     forURL:testURL
                                  isPrivate:false];
  EXPECT_TRUE([braveShieldsUtils blockScriptsEnabledForURL:testURL
                                                 isPrivate:false]);
}

TEST_F(BraveShieldsUtilsTest, TestBlockFingerprinting) {
  auto* braveShieldsUtils =
      [[BraveShieldsUtilsIOS alloc] initWithBrowserState:profile_.get()];
  NSURL* testURL = [NSURL URLWithString:@"https://www.example.com"];
  EXPECT_TRUE([braveShieldsUtils blockFingerprintingEnabledForURL:testURL
                                                        isPrivate:false]);
  [braveShieldsUtils setBlockFingerprintingEnabled:false
                                            forURL:testURL
                                         isPrivate:false];
  EXPECT_FALSE([braveShieldsUtils blockFingerprintingEnabledForURL:testURL
                                                         isPrivate:false]);
}

TEST_F(BraveShieldsUtilsTest, TestAutoShredMode) {
  auto* braveShieldsUtils =
      [[BraveShieldsUtilsIOS alloc] initWithBrowserState:profile_.get()];
  NSURL* testURL = [NSURL URLWithString:@"https://www.example.com"];
  EXPECT_EQ([braveShieldsUtils autoShredModeForURL:testURL isPrivate:false],
            BraveShieldsAutoShredModeNever);
  [braveShieldsUtils setAutoShredMode:BraveShieldsAutoShredModeAppExit
                               forURL:testURL
                            isPrivate:false];
  EXPECT_EQ([braveShieldsUtils autoShredModeForURL:testURL isPrivate:false],
            BraveShieldsAutoShredModeAppExit);
  [braveShieldsUtils setAutoShredMode:BraveShieldsAutoShredModeTabClose
                               forURL:testURL
                            isPrivate:false];
  EXPECT_EQ([braveShieldsUtils autoShredModeForURL:testURL isPrivate:false],
            BraveShieldsAutoShredModeTabClose);
}
