/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_bookmark_provider.h"

#include <memory>
#include <string_view>

#include "base/memory/scoped_refptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/omnibox/browser/brave_fake_autocomplete_provider_client.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engines_test_environment.h"
#include "components/search_engines/template_url_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

class BraveBookmarkProviderTest : public testing::Test {
 public:
  BraveBookmarkProviderTest()
      : model_(bookmarks::TestBookmarkClient::CreateModel()) {}

  AutocompleteInput CreateAutocompleteInput(std::string_view text) {
    AutocompleteInput input(base::UTF8ToUTF16(text),
                            metrics::OmniboxEventProto::OTHER, classifier_);
    return input;
  }

  void SetUp() override {
    EXPECT_CALL(client_, GetBookmarkModel())
        .WillRepeatedly(testing::Return(model_.get()));
    EXPECT_CALL(client_, GetSchemeClassifier())
        .WillRepeatedly(testing::ReturnRef(classifier_));
    auto* node = client_.GetBookmarkModel()->other_node();
    client_.GetBookmarkModel()->AddURL(node, 0, u"Hello",
                                       GURL("https://example.com"));
    client_.set_template_url_service(
        search_engines_test_environment_.template_url_service());
    provider_ = base::MakeRefCounted<BraveBookmarkProvider>(&client_);
  }

  PrefService* prefs() { return client_.GetPrefs(); }

 protected:
  TestSchemeClassifier classifier_;
  search_engines::SearchEnginesTestEnvironment search_engines_test_environment_;
  BraveFakeAutocompleteProviderClient client_;
  std::unique_ptr<bookmarks::BookmarkModel> model_;
  scoped_refptr<BraveBookmarkProvider> provider_;
};

TEST_F(BraveBookmarkProviderTest, SuggestionsDisabledNoResults) {
  prefs()->SetBoolean(omnibox::kBookmarkSuggestionsEnabled, false);
  provider_->Start(CreateAutocompleteInput("Hello"), true);
  EXPECT_TRUE(provider_->matches().empty());
}

TEST_F(BraveBookmarkProviderTest, SuggestionsEnabledHasResults) {
  prefs()->SetBoolean(omnibox::kBookmarkSuggestionsEnabled, true);
  provider_->Start(CreateAutocompleteInput("Hello"), true);
  EXPECT_FALSE(provider_->matches().empty());
}
