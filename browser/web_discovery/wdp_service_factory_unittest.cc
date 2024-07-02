/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/web_discovery/wdp_service_factory.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/web_discovery/common/features.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

TEST(WDPServiceFactoryTest, PrivateNotCreated) {
  content::BrowserTaskEnvironment task_environment;
  base::test::ScopedFeatureList scoped_features(features::kWebDiscoveryNative);
  auto* browser_process = TestingBrowserProcess::GetGlobal();
  TestingProfileManager profile_manager(browser_process);
  ASSERT_TRUE(profile_manager.SetUp());

  auto* profile = profile_manager.CreateTestingProfile("test");

  EXPECT_TRUE(WDPServiceFactory::GetForBrowserContext(profile));
  EXPECT_FALSE(
      WDPServiceFactory::GetForBrowserContext(profile->GetOffTheRecordProfile(
          Profile::OTRProfileID::CreateUniqueForTesting(), true)));
}

}  // namespace web_discovery
