/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/user_education/brave_user_education_utils.h"

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/user_education/user_education_service.h"
#include "chrome/test/base/testing_profile.h"
#include "components/user_education/common/user_education_data.h"
#include "components/user_education/test/test_user_education_storage_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

class BraveUserEducationUtilsTest : public testing::Test {
 public:
  BraveUserEducationUtilsTest() = default;
  ~BraveUserEducationUtilsTest() override = default;

  void SetUp() override {
    // Enable the features we want to test
    feature_list_.InitWithFeatures(
        {features::kSideBySide, features::kSideBySideLinkMenuNewBadge}, {});
    profile_ = std::make_unique<TestingProfile>();
    service_ = std::make_unique<UserEducationService>(profile_.get(),
                                                      /*allows_promos=*/true);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<UserEducationService> service_;
};

TEST_F(BraveUserEducationUtilsTest, SuppressesBadgesForSideBySideFeature) {
  // Verify initial state - badge should not be suppressed
  auto& storage = service_->user_education_storage_service();
  user_education::NewBadgeData initial_data =
      storage.ReadNewBadgeData(features::kSideBySide);

  // Initial data should have default values
  EXPECT_EQ(0, initial_data.show_count);
  EXPECT_EQ(0, initial_data.used_count);

  // Call the function to suppress badges
  SuppressNewBadgesForFeatures(service_.get());

  // Verify badge data has been modified to suppress the badge
  user_education::NewBadgeData suppressed_data =
      storage.ReadNewBadgeData(features::kSideBySide);

  // Badge should now have maximum counts to suppress display
  EXPECT_EQ(999, suppressed_data.show_count);
  EXPECT_EQ(999, suppressed_data.used_count);
  EXPECT_FALSE(suppressed_data.feature_enabled_time.is_null());
}

TEST_F(BraveUserEducationUtilsTest,
       SuppressesBadgesForSideBySideLinkMenuFeature) {
  // Call the function to suppress badges
  SuppressNewBadgesForFeatures(service_.get());

  // Verify kSideBySideLinkMenuNewBadge is also suppressed
  auto& storage = service_->user_education_storage_service();
  user_education::NewBadgeData data =
      storage.ReadNewBadgeData(features::kSideBySideLinkMenuNewBadge);

  EXPECT_EQ(999, data.show_count);
  EXPECT_EQ(999, data.used_count);
  EXPECT_FALSE(data.feature_enabled_time.is_null());
}

TEST_F(BraveUserEducationUtilsTest, HandlesNullService) {
  // Should not crash with null service
  SuppressNewBadgesForFeatures(nullptr);
  // Test passes if no crash occurs
}

TEST_F(BraveUserEducationUtilsTest, PreservesExistingFeatureEnabledTime) {
  auto& storage = service_->user_education_storage_service();

  // Set up initial data with a specific enabled time
  user_education::NewBadgeData initial_data;
  initial_data.feature_enabled_time = base::Time::Now() - base::Days(5);
  initial_data.show_count = 1;
  initial_data.used_count = 1;
  storage.SaveNewBadgeData(features::kSideBySide, initial_data);

  base::Time original_time = initial_data.feature_enabled_time;

  // Call suppress function
  SuppressNewBadgesForFeatures(service_.get());

  // Verify the enabled time was preserved
  user_education::NewBadgeData suppressed_data =
      storage.ReadNewBadgeData(features::kSideBySide);

  EXPECT_EQ(original_time, suppressed_data.feature_enabled_time);
  EXPECT_EQ(999, suppressed_data.show_count);
  EXPECT_EQ(999, suppressed_data.used_count);
}

TEST_F(BraveUserEducationUtilsTest, InitializesFeatureEnabledTimeWhenNull) {
  // Call suppress function with fresh data (null enabled time)
  SuppressNewBadgesForFeatures(service_.get());

  auto& storage = service_->user_education_storage_service();
  user_education::NewBadgeData data =
      storage.ReadNewBadgeData(features::kSideBySide);

  // Should have initialized the enabled time
  EXPECT_FALSE(data.feature_enabled_time.is_null());
  EXPECT_LE(data.feature_enabled_time, base::Time::Now());
}

}  // namespace brave
