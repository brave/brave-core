// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_service_factory.h"

#include "base/values.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AIChatServiceFactoryTest : public testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
};

TEST_F(AIChatServiceFactoryTest, NotCreatedForOffTheRecordProfiles) {
  EXPECT_TRUE(AIChatServiceFactory::GetForBrowserContext(&profile_));
  EXPECT_FALSE(AIChatServiceFactory::GetForBrowserContext(
      profile_.GetPrimaryOTRProfile(/*create_if_needed=*/true)));
}

TEST_F(AIChatServiceFactoryTest, NotCreatedWhenDisabledByPolicy) {
  profile_.GetTestingPrefService()->SetManagedPref(prefs::kEnabledByPolicy,
                                                   base::Value(false));
  EXPECT_FALSE(AIChatServiceFactory::GetForBrowserContext(&profile_));
}

}  // namespace ai_chat
