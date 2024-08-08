/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/search_engines_test_environment.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_search_conversion {

class BraveSearchConversionTest : public testing::Test {
 public:
  BraveSearchConversionTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    RegisterPrefs(pref_service_.registry());
    auto provider_data = TemplateURLDataFromPrepopulatedEngine(
        TemplateURLPrepopulateData::brave_search);
    brave_search_template_url_ = std::make_unique<TemplateURL>(*provider_data);
    provider_data = TemplateURLDataFromPrepopulatedEngine(
        TemplateURLPrepopulateData::brave_search_tor);
    brave_search_tor_template_url_ =
        std::make_unique<TemplateURL>(*provider_data);
    provider_data = TemplateURLDataFromPrepopulatedEngine(
        TemplateURLPrepopulateData::brave_bing);
    bing_template_url_ = std::make_unique<TemplateURL>(*provider_data);
    provider_data = TemplateURLDataFromPrepopulatedEngine(
        TemplateURLPrepopulateData::duckduckgo);
    ddg_template_url_ = std::make_unique<TemplateURL>(*provider_data);
  }

  void ConfigureDDGAsDefaultProvider() {
    search_engines_test_environment_.template_url_service()
        ->SetUserSelectedDefaultSearchProvider(ddg_template_url_.get());
  }

  void ConfigureBingAsDefaultProvider() {
    search_engines_test_environment_.template_url_service()
        ->SetUserSelectedDefaultSearchProvider(bing_template_url_.get());
  }

  void ConfigureBraveSearchAsDefaultProvider(bool tor) {
    search_engines_test_environment_.template_url_service()
        ->SetUserSelectedDefaultSearchProvider(
            tor ? brave_search_tor_template_url_.get()
                : brave_search_template_url_.get());
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TemplateURL> brave_search_template_url_;
  std::unique_ptr<TemplateURL> brave_search_tor_template_url_;
  std::unique_ptr<TemplateURL> bing_template_url_;
  std::unique_ptr<TemplateURL> ddg_template_url_;
  TestingPrefServiceSimple pref_service_;
  search_engines::SearchEnginesTestEnvironment search_engines_test_environment_;
};

TEST_F(BraveSearchConversionTest, DefaultValueTest) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kOmniboxBanner));
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kOmniboxDDGBanner));
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kNTP));
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));
  EXPECT_EQ(GURL("https://search.brave.com/search?q=brave&action=makeDefault"),
            GetPromoURL(u"brave"));
}

TEST_F(BraveSearchConversionTest, ConversionTypeTest) {
  base::test::ScopedFeatureList feature_list;

  ConfigureBingAsDefaultProvider();

  feature_list.InitAndEnableFeatureWithParameters(
      brave_search_conversion::features::kOmniboxBanner,
      {{brave_search_conversion::features::kBannerTypeParamName, "type_B"}});
  EXPECT_EQ(ConversionType::kBannerTypeB,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  // Check do not conversion when brave search(tor) is set as a default
  // provider.
  ConfigureBraveSearchAsDefaultProvider(false);
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));
  ConfigureBraveSearchAsDefaultProvider(true);
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  ConfigureBingAsDefaultProvider();

  // Check conversion type is set again after 3days passed.
  SetMaybeLater(&pref_service_);
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  task_environment_.AdvanceClock(base::Days(2));
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  task_environment_.AdvanceClock(base::Days(1) + base::Milliseconds(1));

  EXPECT_EQ(ConversionType::kBannerTypeB,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  // Set DDG as a default provider and check banner type is still Type B
  // because |kOmniboxDDGBanner| feature is disabled.
  ConfigureDDGAsDefaultProvider();
  EXPECT_EQ(ConversionType::kBannerTypeB,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  feature_list.Reset();

  feature_list.InitAndEnableFeature(
      brave_search_conversion::features::kOmniboxDDGBanner);

  ConfigureBingAsDefaultProvider();

  // Check no banner as current provider is bing.
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  // Set DDG as a default provider and check banner type again.
  ConfigureDDGAsDefaultProvider();
  EXPECT_EQ(ConversionType::kDDGBannerTypeA,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  feature_list.Reset();

  // Set two types of features together as we use different griffin study for
  // both.
  feature_list.InitWithFeaturesAndParameters(
      {{brave_search_conversion::features::kOmniboxBanner,
        {{brave_search_conversion::features::kBannerTypeParamName, "type_B"}}},
       {brave_search_conversion::features::kOmniboxDDGBanner, {}}},
      {});

  // Set Brave and check no banner.
  ConfigureBraveSearchAsDefaultProvider(false);
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  // Set DDG and check ddg banner.
  ConfigureDDGAsDefaultProvider();
  EXPECT_EQ(ConversionType::kDDGBannerTypeA,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  task_environment_.AdvanceClock(base::Minutes(1));
  EXPECT_EQ(ConversionType::kDDGBannerTypeB,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  task_environment_.AdvanceClock(base::Minutes(1));
  EXPECT_EQ(ConversionType::kDDGBannerTypeC,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  task_environment_.AdvanceClock(base::Minutes(1));
  EXPECT_EQ(ConversionType::kDDGBannerTypeD,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  task_environment_.AdvanceClock(base::Minutes(1));
  EXPECT_EQ(ConversionType::kDDGBannerTypeA,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  // Set bing and check non-ddg banner.
  ConfigureBingAsDefaultProvider();
  EXPECT_EQ(ConversionType::kBannerTypeB,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));

  // Set dismissed.
  SetDismissed(&pref_service_);
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(
                &pref_service_,
                search_engines_test_environment_.template_url_service()));
}

}  // namespace brave_search_conversion
