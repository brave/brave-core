/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/suggested_sites_provider.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "brave/components/omnibox/browser/fake_autocomplete_provider_client.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class SuggestedSitesProviderTest : public testing::Test {
 public:
  SuggestedSitesProviderTest() :
      provider_(new SuggestedSitesProvider(&client_)) {
  }

  AutocompleteInput CreateAutocompleteInput(base::StringPiece text) {
    AutocompleteInput input(base::UTF8ToUTF16(text),
                            metrics::OmniboxEventProto::OTHER,
                            classifier_);
    return input;
  }

  PrefService* prefs() {
    return client_.GetPrefs();
  }

 protected:
  TestSchemeClassifier classifier_;
  FakeAutocompleteProviderClient client_;
  scoped_refptr<SuggestedSitesProvider> provider_;
};

// Checks that the suggested sites list is not empty and that non-ASCII inputs
// do not blow the matcher.
TEST_F(SuggestedSitesProviderTest, SmokeTest) {
  provider_->Start(CreateAutocompleteInput(""), true);
  EXPECT_TRUE(provider_->matches().empty());

  provider_->Start(CreateAutocompleteInput("тест"), false);
  EXPECT_TRUE(provider_->matches().empty());

  provider_->Start(CreateAutocompleteInput("테스트"), false);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(SuggestedSitesProviderTest, FourOrMoreChars) {
  provider_->Start(CreateAutocompleteInput("bit"), false);
  EXPECT_TRUE(provider_->matches().empty());

  // Less than 4 chars no match
  provider_->Start(CreateAutocompleteInput("bitc"), false);
  EXPECT_FALSE(provider_->matches().empty());
}

TEST_F(SuggestedSitesProviderTest, LessThan4IfExact) {
  provider_->Start(CreateAutocompleteInput("bri"), false);
  EXPECT_TRUE(provider_->matches().empty());

  provider_->Start(CreateAutocompleteInput("ltc"), false);
  EXPECT_FALSE(provider_->matches().empty());
}

TEST_F(SuggestedSitesProviderTest, OnlyMatchFromStart) {
  provider_->Start(CreateAutocompleteInput("bitc"), false);
  EXPECT_FALSE(provider_->matches().empty());

  // Suffix of a match doesn't match
  provider_->Start(CreateAutocompleteInput("coin"), false);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(SuggestedSitesProviderTest, NoMatchingWhenPrefIsOff) {
  prefs()->SetBoolean(kBraveSuggestedSiteSuggestionsEnabled, false);
  provider_->Start(CreateAutocompleteInput("bitc"), false);
  EXPECT_TRUE(provider_->matches().empty());
}
