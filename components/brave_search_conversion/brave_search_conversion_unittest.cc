/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_search_conversion {

using ::testing::NiceMock;
using ::testing::Return;

class LocaleHelperMock : public brave_l10n::LocaleHelper {
 public:
  MOCK_CONST_METHOD0(GetLocale, std::string());
};

class BraveSearchConversionTest : public testing::Test {
 public:
  BraveSearchConversionTest() : template_url_service_(nullptr, 0) {}
  void SetUp() override {
    SetMockLocale("en-US");
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

  void SetMockLocale(const std::string& locale) {
    // Set promotion supported locale.
    locale_helper_mock_ = std::make_unique<NiceMock<LocaleHelperMock>>();
    brave_l10n::LocaleHelper::GetInstance()->SetForTesting(
        locale_helper_mock_.get());
    ON_CALL(*locale_helper_mock_, GetLocale()).WillByDefault(Return(locale));
  }

  std::unique_ptr<TemplateURL> brave_search_template_url_;
  std::unique_ptr<TemplateURL> brave_search_tor_template_url_;
  std::unique_ptr<TemplateURL> bing_template_url_;
  std::unique_ptr<LocaleHelperMock> locale_helper_mock_;
  TestingPrefServiceSimple pref_service_;
  TemplateURLService template_url_service_;
};

TEST_F(BraveSearchConversionTest, DefaultValueTest) {
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kOmniboxButton));
  EXPECT_FALSE(base::FeatureList::IsEnabled(features::kOmniboxBanner));
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
  EXPECT_EQ(ConversionType::kBanner,
            GetConversionType(&pref_service_, &template_url_service_));

  // Set dismissed.
  SetDismissed(&pref_service_);
  EXPECT_EQ(ConversionType::kNone,
            GetConversionType(&pref_service_, &template_url_service_));
}

TEST_F(BraveSearchConversionTest, SupportedCountryTest) {
  // Below 5 regions are supported for search conversion promotion.
  EXPECT_TRUE(IsPromotionEnabledCountry("US"));
  EXPECT_TRUE(IsPromotionEnabledCountry("CA"));
  EXPECT_TRUE(IsPromotionEnabledCountry("DE"));
  EXPECT_TRUE(IsPromotionEnabledCountry("FR"));
  EXPECT_TRUE(IsPromotionEnabledCountry("GB"));

  EXPECT_FALSE(IsPromotionEnabledCountry("KR"));
  EXPECT_FALSE(IsPromotionEnabledCountry("AZ"));
}

}  // namespace brave_search_conversion
