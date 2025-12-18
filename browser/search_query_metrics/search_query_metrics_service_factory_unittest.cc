/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/search_query_metrics/search_query_metrics_service_factory.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/search_query_metrics/search_query_metrics_feature.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=SearchQueryMetrics*

namespace metrics {

namespace {

TestingProfile* BuildIncognitoProfile(TestingProfile* profile) {
  TestingProfile::Builder profile_builder;
  return profile_builder.BuildIncognito(profile);
}

}  // namespace

class SearchQueryMetricsServiceFactoryTest : public ::testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;

  TestingProfile profile_;

  base::test::ScopedFeatureList feature_list_;
};

TEST_F(SearchQueryMetricsServiceFactoryTest, CreateService) {
  feature_list_.InitAndEnableFeature(kSearchQueryMetricsFeature);
  EXPECT_TRUE(SearchQueryMetricsServiceFactory::GetForProfile(&profile_));
}

TEST_F(SearchQueryMetricsServiceFactoryTest,
       DoNotCreateServiceIfFeatureIsDisabled) {
  feature_list_.InitAndDisableFeature(kSearchQueryMetricsFeature);
  EXPECT_FALSE(SearchQueryMetricsServiceFactory::GetForProfile(&profile_));
}

TEST_F(SearchQueryMetricsServiceFactoryTest,
       CreateServiceIfShouldReportForIncognito) {
  feature_list_.InitAndEnableFeatureWithParameters(
      kSearchQueryMetricsFeature,
      {{"should_report_for_non_regular_profile", "true"}});
  TestingProfile* const incognito_profile = BuildIncognitoProfile(&profile_);
  EXPECT_TRUE(
      SearchQueryMetricsServiceFactory::GetForProfile(incognito_profile));
}

TEST_F(SearchQueryMetricsServiceFactoryTest,
       DoNotCreateServiceForIncognitoIfFeatureIsDisabled) {
  feature_list_.InitAndDisableFeature(kSearchQueryMetricsFeature);
  TestingProfile* const incognito_profile = BuildIncognitoProfile(&profile_);
  EXPECT_FALSE(
      SearchQueryMetricsServiceFactory::GetForProfile(incognito_profile));
}

TEST_F(SearchQueryMetricsServiceFactoryTest,
       DoNotCreateServiceForIncognitoIfShouldNotReport) {
  feature_list_.InitAndEnableFeatureWithParameters(
      kSearchQueryMetricsFeature,
      {{"should_report_for_non_regular_profile", "false"}});
  TestingProfile* const incognito_profile = BuildIncognitoProfile(&profile_);
  EXPECT_FALSE(
      SearchQueryMetricsServiceFactory::GetForProfile(incognito_profile));
}

}  // namespace metrics
