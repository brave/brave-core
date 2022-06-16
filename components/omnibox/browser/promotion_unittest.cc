/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/omnibox/browser/promotion_provider.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/mock_autocomplete_provider_client.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave_search_conversion::ConversionType;
using brave_search_conversion::GetPromoURL;
using brave_search_conversion::RegisterPrefs;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

class LocaleHelperMock : public brave_l10n::LocaleHelper {
 public:
  MOCK_CONST_METHOD0(GetLocale, std::string());
};

}  // namespace

class OmniboxPromotionTest : public testing::Test {
 public:
  OmniboxPromotionTest() = default;
  ~OmniboxPromotionTest() override = default;

  void SetUp() override {
    RegisterPrefs(pref_service_.registry());
    omnibox::RegisterBraveProfilePrefs(pref_service_.registry());
    pref_service_.SetBoolean(omnibox::kTopSiteSuggestionsEnabled, false);
    pref_service_.SetBoolean(omnibox::kBraveSuggestedSiteSuggestionsEnabled,
                             false);

    SetMockLocale("en-US");
  }

  void SetMockLocale(const std::string& locale) {
    // Set promotion supported locale.
    locale_helper_mock_ = std::make_unique<NiceMock<LocaleHelperMock>>();
    brave_l10n::LocaleHelper::GetInstance()->SetForTesting(
        locale_helper_mock_.get());
    ON_CALL(*locale_helper_mock_, GetLocale()).WillByDefault(Return(locale));
  }

  void CreateController(bool incognito) {
    // Set non brave search provider to get promotion match.
    auto bing_data = TemplateURLDataFromPrepopulatedEngine(
        TemplateURLPrepopulateData::brave_bing);
    auto bing_template_url = std::make_unique<TemplateURL>(*bing_data);
    auto template_url_service =
        std::make_unique<TemplateURLService>(nullptr, 0);
    template_url_service->Load();
    template_url_service->SetUserSelectedDefaultSearchProvider(
        bing_template_url.get());

    auto client_mock = std::make_unique<MockAutocompleteProviderClient>();
    client_mock->set_template_url_service(std::move(template_url_service));
    ON_CALL(*client_mock, GetPrefs()).WillByDefault(Return(&pref_service_));
    ON_CALL(*client_mock, IsOffTheRecord()).WillByDefault(Return(incognito));
    controller_ = std::make_unique<AutocompleteController>(
        std::move(client_mock), AutocompleteProvider::TYPE_SEARCH);
  }

  // Give 4 matches.
  ACMatches CreateTestMatches() {
    ACMatches matches;
    // Make first item is search query with default provider.
    AutocompleteMatch match(nullptr, 800, true,
                            AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED);
    matches.push_back(match);
    match.type = AutocompleteMatchType::NAVSUGGEST;
    matches.push_back(match);
    matches.push_back(match);
    matches.push_back(match);
    return matches;
  }

  bool HasPromotionMatch() {
    for (const auto& match : controller_->result()) {
      if (IsBraveSearchPromotionMatch(match)) {
        return true;
      }
    }
    return false;
  }

  content::BrowserTaskEnvironment browser_task_environment_;
  TestSchemeClassifier classifier_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<AutocompleteController> controller_;
};

// Promotion match should not be added for private profile.
TEST_F(OmniboxPromotionTest, ProfileTest) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_search_conversion::features::kOmniboxButton);
  CreateController(false);
  AutocompleteInput input(u"brave", metrics::OmniboxEventProto::OTHER,
                          classifier_);
  controller_->Start(input);
  EXPECT_TRUE(HasPromotionMatch());

  CreateController(true);
  controller_->Start(input);
  EXPECT_FALSE(HasPromotionMatch());
}

TEST_F(OmniboxPromotionTest, PromotionProviderTest) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_search_conversion::features::kOmniboxButton);

  CreateController(false);
  EXPECT_TRUE(controller_->result().empty());
  AutocompleteInput input(u"brave", metrics::OmniboxEventProto::OTHER,
                          classifier_);
  controller_->Start(input);
  int promotion_match_count = 0;
  for (const auto& match : controller_->result()) {
    if (IsBraveSearchPromotionMatch(match)) {
      promotion_match_count++;
    }
  }
  EXPECT_EQ(1, promotion_match_count);

  // Check promotion match is not added.
  feature_list.Reset();
  controller_->Start(input);
  EXPECT_FALSE(HasPromotionMatch());
}

TEST_F(OmniboxPromotionTest, AutocompleteResultTest) {
  AutocompleteInput input(u"brave", metrics::OmniboxEventProto::OTHER,
                          classifier_);

  AutocompleteResult result;
  ACMatches matches = CreateTestMatches();
  // Make 4th match as button type promotion and check that promotion is
  // reordered at second.
  matches[3].destination_url = GetPromoURL(input.text());
  SetConversionTypeToMatch(ConversionType::kButton, &matches[3]);
  result.AppendMatches(matches);
  SortBraveSearchPromotionMatch(&result);
  EXPECT_TRUE(IsBraveSearchPromotionMatch(*result.match_at(1)));

  result.Reset();
  matches = CreateTestMatches();
  // Make 3rd match as banner type promotion and check that promotion is
  // reordered at last.
  matches[2].destination_url = GetPromoURL(input.text());
  SetConversionTypeToMatch(ConversionType::kBanner, &matches[2]);
  result.AppendMatches(matches);
  SortBraveSearchPromotionMatch(&result);
  EXPECT_TRUE(IsBraveSearchPromotionMatch(*result.match_at(3)));

  result.Reset();
  matches = CreateTestMatches();
  matches[2].destination_url = GetPromoURL(input.text());
  SetConversionTypeToMatch(ConversionType::kBanner, &matches[2]);
  result.AppendMatches(matches);
  // Make first match is not search query with default provider.
  result.begin()->type = AutocompleteMatchType::NAVSUGGEST;
  // Check promotion match is deleted from |result| when
  SortBraveSearchPromotionMatch(&result);
  for (const auto& match : result) {
    EXPECT_FALSE(IsBraveSearchPromotionMatch(match));
  }
}
