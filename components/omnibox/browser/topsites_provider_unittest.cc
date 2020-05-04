/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/topsites_provider.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "brave/components/omnibox/browser/fake_autocomplete_provider_client.h"
#include "components/omnibox/browser/mock_autocomplete_provider_client.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class TopSitesProviderTest : public testing::Test {
 public:
  TopSitesProviderTest() : provider_(new TopSitesProvider(&client_)) {
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
  scoped_refptr<TopSitesProvider> provider_;
};

// Checks that the top sites list is not empty and that non-ASCII inputs do not
// blow the matcher.
TEST_F(TopSitesProviderTest, SmokeTest) {
  provider_->Start(CreateAutocompleteInput(""), false);
  EXPECT_TRUE(provider_->matches().empty());

  provider_->Start(CreateAutocompleteInput("dex"), false);
  EXPECT_FALSE(provider_->matches().empty());

  provider_->Start(CreateAutocompleteInput("тест"), false);
  EXPECT_TRUE(provider_->matches().empty());

  provider_->Start(CreateAutocompleteInput("테스트"), false);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(TopSitesProviderTest, NoMatchingWhenPrefIsOff) {
  prefs()->SetBoolean(kTopSiteSuggestionsEnabled, false);
  provider_->Start(CreateAutocompleteInput("dex"), false);
  EXPECT_TRUE(provider_->matches().empty());
}
