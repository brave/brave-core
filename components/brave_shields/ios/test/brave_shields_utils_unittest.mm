// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <Foundation/Foundation.h>

#include "base/test/task_environment.h"
#include "brave/components/brave_shields/ios/browser/brave_shields_utils_impl.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

class BraveShieldsUtilsTest : public PlatformTest {
 public:
  explicit BraveShieldsUtilsTest()
      : task_environment_(
            base::test::TaskEnvironment::ThreadingMode::MAIN_THREAD_ONLY) {
    host_content_settings_map_ = base::MakeRefCounted<HostContentSettingsMap>(
        &pref_service_, /*is_off_the_record=*/false,
        /*store_last_modified=*/false, /*restore_session=*/false,
        /*should_record_metrics=*/false);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::test::SingleThreadTaskEnvironment single_task_environment_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  scoped_refptr<HostContentSettingsMap> host_content_settings_map_;
};

TEST_F(BraveShieldsUtilsTest, TestBraveShieldsEnabled) {
  auto* braveShieldsUtils = [[BraveShieldsUtilsImpl alloc]
      initWithHostContentSettingsMap:host_content_settings_map_.get()
                          localState:&pref_service_
                        profilePrefs:&pref_service_];
  NSURL* testURL = [NSURL URLWithString:@"https://www.example.com"];
  EXPECT_TRUE([braveShieldsUtils braveShieldsEnabledFor:testURL]);
  [braveShieldsUtils setBraveShieldsEnabled:false forURL:testURL];
  EXPECT_FALSE([braveShieldsUtils braveShieldsEnabledFor:testURL]);
  EXPECT_TRUE(true);
}

TEST_F(BraveShieldsUtilsTest, TestAdBlockMode) {
  auto* braveShieldsUtils = [[BraveShieldsUtilsImpl alloc]
      initWithHostContentSettingsMap:host_content_settings_map_.get()
                          localState:&pref_service_
                        profilePrefs:&pref_service_];
  NSURL* testURL = [NSURL URLWithString:@"https://www.example.com"];
  EXPECT_EQ([braveShieldsUtils adBlockModeForURL:testURL], BraveShieldsAdBlockModeStandard);
  [braveShieldsUtils setAdBlockMode:BraveShieldsAdBlockModeAggressive forURL:testURL];
  EXPECT_EQ([braveShieldsUtils adBlockModeForURL:testURL], BraveShieldsAdBlockModeAggressive);
  [braveShieldsUtils setAdBlockMode:BraveShieldsAdBlockModeAllow forURL:testURL];
  EXPECT_EQ([braveShieldsUtils adBlockModeForURL:testURL], BraveShieldsAdBlockModeAllow);
}

TEST_F(BraveShieldsUtilsTest, TestBlockScripts) {
  auto* braveShieldsUtils = [[BraveShieldsUtilsImpl alloc]
      initWithHostContentSettingsMap:host_content_settings_map_.get()
                          localState:&pref_service_
                        profilePrefs:&pref_service_];
  NSURL* testURL = [NSURL URLWithString:@"https://www.example.com"];
  EXPECT_FALSE([braveShieldsUtils blockScriptsEnabledForURL:testURL]);
  [braveShieldsUtils setBlockScriptsEnabled:false forURL:testURL];
  EXPECT_TRUE([braveShieldsUtils blockScriptsEnabledForURL:testURL]);
}

// TEST_F(BraveShieldsUtilsTest, TestBlockFingerprinting) {
//   auto* braveShieldsUtils = [[BraveShieldsUtilsImpl alloc]
//       initWithHostContentSettingsMap:host_content_settings_map_.get()
//                           localState:&pref_service_
//                         profilePrefs:&pref_service_];
//   NSURL* testURL = [NSURL URLWithString:@"https://www.example.com"];
//   EXPECT_TRUE([braveShieldsUtils blockFingerprintingEnabledForURL:testURL]);
//   [braveShieldsUtils setBlockFingerprintingEnabled:false forURL:testURL];
//   EXPECT_FALSE([braveShieldsUtils blockFingerprintingEnabledForURL:testURL]);
// }

TEST_F(BraveShieldsUtilsTest, TestAutoShredMode) {
  auto* braveShieldsUtils = [[BraveShieldsUtilsImpl alloc]
      initWithHostContentSettingsMap:host_content_settings_map_.get()
                          localState:&pref_service_
                        profilePrefs:&pref_service_];
  NSURL* testURL = [NSURL URLWithString:@"https://www.example.com"];
  EXPECT_EQ([braveShieldsUtils autoShredModeForURL:testURL], BraveShieldsAutoShredModeNever);
  [braveShieldsUtils setAutoShredMode:BraveShieldsAutoShredModeAppExit forURL:testURL];
  EXPECT_EQ([braveShieldsUtils autoShredModeForURL:testURL], BraveShieldsAutoShredModeAppExit);
  [braveShieldsUtils setAutoShredMode:BraveShieldsAutoShredModeTabClose forURL:testURL];
  EXPECT_EQ([braveShieldsUtils autoShredModeForURL:testURL], BraveShieldsAutoShredModeTabClose);
}