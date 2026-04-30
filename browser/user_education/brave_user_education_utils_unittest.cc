/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/user_education/brave_user_education_utils.h"

#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/user_education/user_education_service.h"
#include "chrome/test/base/testing_profile.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/user_education/common/feature_promo/feature_promo_lifecycle.h"
#include "components/user_education/common/feature_promo/feature_promo_result.h"
#include "components/user_education/common/feature_promo/feature_promo_specification.h"
#include "components/user_education/common/new_badge/new_badge_policy.h"
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
        {feature_engagement::kIPHSideBySidePinnableFeature,
         feature_engagement::kIPHSideBySideTabSwitchFeature},
        {});
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

TEST_F(BraveUserEducationUtilsTest, HandlesNullService) {
  // Should not crash with null service
  SuppressUserEducation(nullptr);
  // Test passes if no crash occurs
}

TEST_F(BraveUserEducationUtilsTest, SuppressesIPHForSideBySidePinnableFeature) {
  auto& storage = service_->user_education_storage_service();

  // Verify initial state - promo should not be dismissed
  auto initial_data =
      storage.ReadPromoData(feature_engagement::kIPHSideBySidePinnableFeature);
  EXPECT_FALSE(initial_data.has_value());

  // Call the function to suppress promos
  SuppressUserEducation(service_.get());

  // Verify promo data has been modified to suppress the promo
  auto suppressed_data =
      storage.ReadPromoData(feature_engagement::kIPHSideBySidePinnableFeature);

  ASSERT_TRUE(suppressed_data.has_value());
  EXPECT_TRUE(suppressed_data->is_dismissed);
}

TEST_F(BraveUserEducationUtilsTest,
       SuppressesIPHForSideBySideTabSwitchFeature) {
  auto& storage = service_->user_education_storage_service();

  // Verify initial state - promo should not be dismissed
  auto initial_data =
      storage.ReadPromoData(feature_engagement::kIPHSideBySideTabSwitchFeature);
  EXPECT_FALSE(initial_data.has_value());

  // Call the function to suppress promos
  SuppressUserEducation(service_.get());

  // Verify promo data has been modified to suppress the promo
  auto suppressed_data =
      storage.ReadPromoData(feature_engagement::kIPHSideBySideTabSwitchFeature);

  ASSERT_TRUE(suppressed_data.has_value());
  EXPECT_TRUE(suppressed_data->is_dismissed);
}

TEST_F(BraveUserEducationUtilsTest, PromoShouldBePermanentlyDismissed) {
  SuppressUserEducation(service_.get());

  auto& storage = service_->user_education_storage_service();

  // Set profile creation time to the past to avoid new profile grace period.
  storage.set_profile_creation_time_for_testing(base::Time::Now() -
                                                base::Days(30));

  // Create a lifecycle to verify CanShow() returns kPermanentlyDismissed.
  user_education::FeaturePromoLifecycle lifecycle(
      &storage,
      /*promo_key=*/"", &feature_engagement::kIPHSideBySidePinnableFeature,
      user_education::FeaturePromoSpecification::PromoType::kToast,
      user_education::FeaturePromoSpecification::PromoSubtype::kNormal,
      /*num_rotating_entries=*/0);

  EXPECT_EQ(user_education::FeaturePromoResult::kPermanentlyDismissed,
            lifecycle.CanShow());
}

TEST_F(BraveUserEducationUtilsTest, PromoBlockedByNewProfile) {
  // Don't suppress - just test that new profiles block promos.
  auto& storage = service_->user_education_storage_service();

  // Profile creation time defaults to now (new profile).
  storage.set_profile_creation_time_for_testing(base::Time::Now());

  user_education::FeaturePromoLifecycle lifecycle(
      &storage,
      /*promo_key=*/"", &feature_engagement::kIPHSideBySidePinnableFeature,
      user_education::FeaturePromoSpecification::PromoType::kToast,
      user_education::FeaturePromoSpecification::PromoSubtype::kNormal,
      /*num_rotating_entries=*/0);

  // New profiles should block normal promos during grace period.
  EXPECT_EQ(user_education::FeaturePromoResult::kBlockedByNewProfile,
            lifecycle.CanShow());
}

}  // namespace brave
