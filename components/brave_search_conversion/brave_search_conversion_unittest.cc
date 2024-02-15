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
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_search_conversion {

class BraveSearchConversionTest : public testing::Test {
 public:
  BraveSearchConversionTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        template_url_service_(
            static_cast<TemplateURLService::Initializer*>(nullptr),
            0u) {}

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

    PrepareFieldTrialParamsForBannerTypeA();
  }

  void PrepareFieldTrialParamsForBannerTypeA() {
    constexpr char kPromotionTrial[] = "BraveSearchPromotionBannerStudy";
    constexpr char kBannerTypeParamName[] = "banner_type";
    constexpr char kBannerTypeExperiements[] = "banner_type_a";

    std::map<std::string, std::string> params;
    params[kBannerTypeParamName] = "type_A";
    ASSERT_TRUE(base::AssociateFieldTrialParams(
        kPromotionTrial, kBannerTypeExperiements, params));
    base::FieldTrialList::CreateFieldTrial(kPromotionTrial,
                                           kBannerTypeExperiements);
  }

  void ConfigureBingAsDefaultProvider() {
    template_url_service_.SetUserSelectedDefaultSearchProvider(
        bing_template_url_.get());
  }

  void ConfigureBraveSearchAsDefaultProvider(bool tor) {
    template_url_service_.SetUserSelectedDefaultSearchProvider(
        tor ? brave_search_tor_template_url_.get()
            : brave_search_template_url_.get());
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TemplateURL> brave_search_template_url_;
  std::unique_ptr<TemplateURL> brave_search_tor_template_url_;
  std::unique_ptr<TemplateURL> bing_template_url_;
  TestingPrefServiceSimple pref_service_;
  TemplateURLService template_url_service_;
};

TEST_F(BraveSearchConversionTest, DefaultValueTest) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kOmniboxButton));
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kOmniboxBanner));
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kNTP));
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(&pref_service_, &template_url_service_));
  EXPECT_EQ(GURL("https://search.brave.com/search?q=brave&action=makeDefault"),
            GetPromoURL(u"brave"));
}

TEST_F(BraveSearchConversionTest, ConversionTypeTest) {
  base::test::ScopedFeatureList feature_list;

  ConfigureBingAsDefaultProvider();

  feature_list.InitAndEnableFeature(features::kOmniboxButton);
  EXPECT_EQ(ConversionType::kButton,
            GetConversionType(&pref_service_, &template_url_service_));

  // Check do not conversion when brave search(tor) is set as a default
  // provider.
  ConfigureBraveSearchAsDefaultProvider(false);
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(&pref_service_, &template_url_service_));
  ConfigureBraveSearchAsDefaultProvider(true);
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(&pref_service_, &template_url_service_));

  ConfigureBingAsDefaultProvider();

  feature_list.Reset();
  feature_list.InitAndEnableFeature(features::kOmniboxBanner);
  EXPECT_EQ(ConversionType::kBannerTypeA,
            GetConversionType(&pref_service_, &template_url_service_));

  // Check conversion type is set again after 3days passed.
  SetMaybeLater(&pref_service_);
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(&pref_service_, &template_url_service_));

  task_environment_.AdvanceClock(base::Days(2));
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(&pref_service_, &template_url_service_));

  task_environment_.AdvanceClock(base::Days(1) + base::Milliseconds(1));
  EXPECT_EQ(ConversionType::kBannerTypeA,
            GetConversionType(&pref_service_, &template_url_service_));

  // Set dismissed.
  SetDismissed(&pref_service_);
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(&pref_service_, &template_url_service_));
}

}  // namespace brave_search_conversion
