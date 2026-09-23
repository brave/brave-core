// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_search/backup_results_service_factory.h"

#include "brave/components/brave_search/browser/backup_results_service.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_search {

class BackupResultsServiceFactoryTest : public testing::Test {
 protected:
  void SetUp() override { ASSERT_TRUE(profile_manager_.SetUp()); }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager profile_manager_{TestingBrowserProcess::GetGlobal()};
};

// The service must only be available for non-OTR profiles.
TEST_F(BackupResultsServiceFactoryTest, NoServiceForOffTheRecordProfile) {
  auto* profile = profile_manager_.CreateTestingProfile("test");
  ASSERT_TRUE(profile);
  auto* otr_profile = profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  ASSERT_TRUE(otr_profile->IsOffTheRecord());

  EXPECT_NE(BackupResultsServiceFactory::GetForBrowserContext(profile),
            nullptr);
  EXPECT_EQ(BackupResultsServiceFactory::GetForBrowserContext(otr_profile),
            nullptr);
}

}  // namespace brave_search
