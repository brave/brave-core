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
#include "brave/components/omnibox/browser/promotion_provider.h"
#include "brave/components/omnibox/browser/promotion_utils.h"
#include "brave/components/search_engines/brave_prepopulated_engines.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_result.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/search_engines/template_url_service.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave_search_conversion::ConversionType;
using brave_search_conversion::GetPromoURL;
using brave_search_conversion::RegisterPrefs;

class OmniboxPromotionTest : public testing::Test {
 public:
  OmniboxPromotionTest() : template_url_service_(nullptr, 0) {}
  ~OmniboxPromotionTest() override = default;

  void SetUp() override {
    brave_search_conversion::RegisterPrefs(pref_service_.registry());
    // Set non brave search provider to get promotion match.
    auto provider_data = TemplateURLDataFromPrepopulatedEngine(
        TemplateURLPrepopulateData::brave_bing);
    auto brave_search_template_url =
        std::make_unique<TemplateURL>(*provider_data);

    template_url_service_.Load();
    template_url_service_.SetUserSelectedDefaultSearchProvider(
        brave_search_template_url.get());

    provider_ = new PromotionProvider(&pref_service_, &template_url_service_);
  }

  TestingPrefServiceSimple pref_service_;
  TemplateURLService template_url_service_;
  TestSchemeClassifier classifier_;
  scoped_refptr<PromotionProvider> provider_;
};

TEST_F(OmniboxPromotionTest, PromotionProviderTest) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      brave_search_conversion::features::kOmniboxButton);

  EXPECT_TRUE(provider_->matches().empty());
  AutocompleteInput input(u"brave", metrics::OmniboxEventProto::OTHER,
                          classifier_);
  provider_->Start(input, false);
  // Get one result.
  EXPECT_EQ(1UL, provider_->matches().size());

  // Check promotion match is not added.
  feature_list.Reset();
  provider_->Start(input, false);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(OmniboxPromotionTest, AutocompleteResultTest) {
  ACMatches matches;
  AutocompleteInput input(u"brave", metrics::OmniboxEventProto::OTHER,
                          classifier_);

  // Make first item is search query with default provider.
  AutocompleteMatch match(nullptr, 800, true,
                          AutocompleteMatchType::SEARCH_WHAT_YOU_TYPED);
  matches.push_back(match);
  match.type = AutocompleteMatchType::NAVSUGGEST;
  matches.push_back(match);
  matches.push_back(match);
  // Put promo match as 4th match.
  match.destination_url = GetPromoURL(input.text());
  matches.push_back(match);

  match.destination_url = GURL();
  matches.push_back(match);

  AutocompleteResult result;
  result.AppendMatches(input, matches);
  SortBraveSearchPromotionMatch(&result, input, ConversionType::kNone);
  // Match position is not changed for kNone type.
  EXPECT_TRUE(IsBraveSearchPromotionMatch(*result.match_at(3), input.text()));

  // Check match is re-positioned as a second match for button type.
  SortBraveSearchPromotionMatch(&result, input, ConversionType::kButton);
  EXPECT_TRUE(IsBraveSearchPromotionMatch(*result.match_at(1), input.text()));

  // Check match is re-positioned as a last match for banner type.
  SortBraveSearchPromotionMatch(&result, input, ConversionType::kBanner);
  EXPECT_TRUE(IsBraveSearchPromotionMatch(*result.match_at(4), input.text()));

  // Make first match is not search query with default provider.
  result.begin()->type = AutocompleteMatchType::NAVSUGGEST;
  // Check promotion match is deleted from |result|.
  SortBraveSearchPromotionMatch(&result, input, ConversionType::kButton);
  for (const auto& match : result) {
    EXPECT_FALSE(IsBraveSearchPromotionMatch(match, input.text()));
  }
}
