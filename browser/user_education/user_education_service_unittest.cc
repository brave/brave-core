/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/user_education/user_education_service.h"

#include "base/files/scoped_temp_dir.h"
#include "chrome/browser/user_education/user_education_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class UserEducationServiceTest : public testing::Test {
 public:
  UserEducationServiceTest() = default;
  UserEducationServiceTest(const UserEducationServiceTest&) = delete;
  UserEducationServiceTest& operator=(const UserEducationServiceTest&) = delete;

  void SetUp() override {
    ASSERT_TRUE(profile_dir_.CreateUniqueTempDir());
    TestingProfile::Builder profile_builder;
    profile_builder.SetPath(profile_dir_.GetPath());
    profile_ = profile_builder.Build();
  }

  Profile* profile() { return profile_.get(); }

 private:
  base::ScopedTempDir profile_dir_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(UserEducationServiceTest, NoUserEducationService) {
  auto* service = UserEducationServiceFactory::GetForBrowserContext(profile());
  EXPECT_EQ(nullptr, service);
}
