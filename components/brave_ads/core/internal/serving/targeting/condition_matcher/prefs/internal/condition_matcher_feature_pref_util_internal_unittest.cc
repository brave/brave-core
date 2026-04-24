/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_feature_pref_util_internal.h"

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_all_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
BASE_FEATURE(kTestFeature, base::FEATURE_ENABLED_BY_DEFAULT);
}  // namespace

class BraveAdsConditionMatcherFeaturePrefUtilInternalTest
    : public test::TestBase {};

TEST_F(BraveAdsConditionMatcherFeaturePrefUtilInternalTest,
       GetIsOverriddenWhenFeatureIsOverridden) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kTestFeature);

  // Act
  std::optional<base::Value> feature_dict =
      MaybeGetFeaturePrefValue("[virtual]:feature=TestFeature");

  // Assert
  ASSERT_TRUE(feature_dict);
  EXPECT_EQ("1", *feature_dict->GetDict().FindString("is_overridden"));
}

TEST_F(BraveAdsConditionMatcherFeaturePrefUtilInternalTest,
       GetIsNotOverriddenWhenFeatureIsNotOverridden) {
  // Act
  std::optional<base::Value> feature_dict =
      MaybeGetFeaturePrefValue("[virtual]:feature=TestFeature");

  // Assert
  ASSERT_TRUE(feature_dict);
  EXPECT_EQ("0", *feature_dict->GetDict().FindString("is_overridden"));
}

TEST_F(BraveAdsConditionMatcherFeaturePrefUtilInternalTest,
       GetParamsWhenFeatureHasTrial) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(kTestFeature,
                                                         {{"foo", "bar"}});

  // Act
  std::optional<base::Value> feature_dict =
      MaybeGetFeaturePrefValue("[virtual]:feature=TestFeature");

  // Assert
  ASSERT_TRUE(feature_dict);
  const auto* const params_dict = feature_dict->GetDict().FindDict("params");
  ASSERT_TRUE(params_dict);
  EXPECT_EQ("bar", *params_dict->FindString("foo"));
}

TEST_F(BraveAdsConditionMatcherFeaturePrefUtilInternalTest,
       DoNotGetParamsDictWhenFeatureHasNoTrial) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(kTestFeature);

  // Act
  std::optional<base::Value> feature_dict =
      MaybeGetFeaturePrefValue("[virtual]:feature=TestFeature");

  // Assert
  ASSERT_TRUE(feature_dict);
  EXPECT_FALSE(feature_dict->GetDict().FindDict("params"));
}

TEST_F(BraveAdsConditionMatcherFeaturePrefUtilInternalTest,
       DoNotGetFeaturePrefValueForNonKeywordPath) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetFeaturePrefValue("TestFeature"));
}

TEST_F(BraveAdsConditionMatcherFeaturePrefUtilInternalTest,
       DoNotGetFeaturePrefValueForEmptyFeatureName) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetFeaturePrefValue("[virtual]:feature="));
}

TEST_F(BraveAdsConditionMatcherFeaturePrefUtilInternalTest,
       DoNotGetFeaturePrefValueForKeywordWithNoEqualsSign) {
  // Act & Assert
  EXPECT_FALSE(MaybeGetFeaturePrefValue("[virtual]:feature"));
}

}  // namespace brave_ads
