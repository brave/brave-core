/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <iterator>
#include <memory>

#include "base/memory/scoped_refptr.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/ranges/algorithm.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/omnibox/browser/promotion_provider.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/omnibox/browser/autocomplete_controller.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
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
using ::testing::Eq;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

// Provides 2 dummy matches to make multiple matches for testing promotion
// entry position sorting.
class DummyProvider : public AutocompleteProvider {
 public:
  explicit DummyProvider(AutocompleteProvider::Type type)
      : AutocompleteProvider(type) {}
  DummyProvider(const DummyProvider&) = delete;
  DummyProvider& operator=(const DummyProvider&) = delete;

  // AutocompleteProvider overrides:
  void Start(const AutocompleteInput& input, bool minimal_changes) override {
    if (type_ == AutocompleteProvider::TYPE_SEARCH) {
      AutocompleteMatch match(nullptr, 800, true,
                              AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED);
      match.keyword = u"brave";
      matches_.push_back(match);

      match.keyword = u"browser";
      matches_.push_back(match);
    } else {
      AutocompleteMatch match(nullptr, 600, true,
                              AutocompleteMatchType::BOOKMARK_TITLE);
      matches_.push_back(match);
      matches_.push_back(match);
    }
  }

 private:
  ~DummyProvider() override = default;
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

    scoped_default_locale_ =
        std::make_unique<brave_l10n::test::ScopedDefaultLocale>("en_US");
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
    controller_->providers_.push_back(
        new DummyProvider(AutocompleteProvider::TYPE_SEARCH));
    controller_->providers_.push_back(
        new DummyProvider(AutocompleteProvider::TYPE_BOOKMARK));
  }

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
  std::unique_ptr<AutocompleteController> controller_;
  std::unique_ptr<brave_l10n::test::ScopedDefaultLocale> scoped_default_locale_;
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

TEST_F(OmniboxPromotionTest, PromotionEntrySortTest) {
  constexpr int kTotalMatchCount = 6;
  base::test::ScopedFeatureList feature_list_button;

  // Test button type search conversion.
  feature_list_button.InitAndEnableFeature(
      brave_search_conversion::features::kOmniboxButton);

  CreateController(false);
  EXPECT_TRUE(controller_->result().empty());
  AutocompleteInput input(u"brave", metrics::OmniboxEventProto::OTHER,
                          classifier_);
  controller_->Start(input);
  EXPECT_EQ(static_cast<size_t>(kTotalMatchCount),
            controller_->result().size());
  int promotion_match_count = 0;
  for (const auto& match : controller_->result()) {
    if (IsBraveSearchPromotionMatch(match)) {
      promotion_match_count++;
    }
  }

  // There is one button promotion entry.
  EXPECT_EQ(1, promotion_match_count);

  auto brave_search_conversion_match =
      base::ranges::find_if(controller_->result(), IsBraveSearchPromotionMatch);
  EXPECT_NE(brave_search_conversion_match, controller_->result().end());

  // Located as second entry for button type.
  EXPECT_EQ(1, std::distance(controller_->result().begin(),
                             brave_search_conversion_match));

  // Turn off button type and on banner type search conversion.
  feature_list_button.Reset();
  base::test::ScopedFeatureList feature_list_banner;
  feature_list_banner.InitAndEnableFeatureWithParameters(
      brave_search_conversion::features::kOmniboxBanner,
      {{brave_search_conversion::features::kBannerTypeParamName, "type_B"}});

  CreateController(false);
  EXPECT_TRUE(controller_->result().empty());
  controller_->Start(input);
  promotion_match_count = 0;
  for (const auto& match : controller_->result()) {
    if (IsBraveSearchPromotionMatch(match)) {
      promotion_match_count++;
    }
  }

  // There is one banner promotion entry.
  EXPECT_EQ(1, promotion_match_count);

  brave_search_conversion_match =
      base::ranges::find_if(controller_->result(), IsBraveSearchPromotionMatch);
  EXPECT_NE(brave_search_conversion_match, controller_->result().end());

  // Located as last entry for banner type.
  EXPECT_EQ(kTotalMatchCount - 1, std::distance(controller_->result().begin(),
                                                brave_search_conversion_match));

  feature_list_banner.Reset();

  // Check promotion match is not added when feature is off.
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
  SetConversionTypeToMatch(ConversionType::kBannerTypeA, &matches[2]);
  result.AppendMatches(matches);
  SortBraveSearchPromotionMatch(&result);
  EXPECT_TRUE(IsBraveSearchPromotionMatch(*result.match_at(3)));

  result.Reset();
  matches = CreateTestMatches();
  matches[2].destination_url = GetPromoURL(input.text());
  SetConversionTypeToMatch(ConversionType::kBannerTypeA, &matches[2]);
  result.AppendMatches(matches);
  // Make first match is not search query with default provider.
  result.begin()->type = AutocompleteMatchType::NAVSUGGEST;
  // Check promotion match is deleted from |result| when
  SortBraveSearchPromotionMatch(&result);
  for (const auto& match : result) {
    EXPECT_FALSE(IsBraveSearchPromotionMatch(match));
  }
}
