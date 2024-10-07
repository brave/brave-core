// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/omnibox/browser/brave_local_history_zero_suggest_provider.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/omnibox/browser/fake_autocomplete_provider_client.h"
#include "components/omnibox/browser/history_quick_provider.h"
#include "components/omnibox/browser/in_memory_url_index.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/search_engines/prepopulated_engines.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data_util.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class BraveLocalHistoryZeroSuggestProviderTest
    : public testing::Test,
      public AutocompleteProviderListener {
 public:
  BraveLocalHistoryZeroSuggestProviderTest() {}

  ~BraveLocalHistoryZeroSuggestProviderTest() override {}

  BraveLocalHistoryZeroSuggestProviderTest(
      const BraveLocalHistoryZeroSuggestProviderTest&) = delete;
  BraveLocalHistoryZeroSuggestProviderTest& operator=(
      const BraveLocalHistoryZeroSuggestProviderTest&) = delete;

  AutocompleteInput CreateAutocompleteInput(std::string_view text) {
    AutocompleteInput input(u"", metrics::OmniboxEventProto::NTP_REALBOX,
                            TestSchemeClassifier());
    input.set_focus_type(metrics::OmniboxFocusType::INTERACTION_FOCUS);
    return input;
  }

  void AddSearchTerm(const std::string& search_terms, int age_in_seconds) {
    TemplateURLRef::SearchTermsArgs search_term_args(
        base::UTF8ToUTF16(search_terms));
    const auto& search_terms_data =
        client_->GetTemplateURLService()->search_terms_data();
    std::string search_url =
        default_search_provider()->url_ref().ReplaceSearchTerms(
            search_term_args, search_terms_data);
    client_->GetHistoryService()->AddPageWithDetails(
        GURL(search_url), base::UTF8ToUTF16(search_terms), 1, 1,
        base::Time::Now() - base::Seconds(age_in_seconds), false,
        history::SOURCE_BROWSED);
    client_->GetHistoryService()->SetKeywordSearchTermsForURL(
        GURL(search_url), default_search_provider()->id(),
        base::UTF8ToUTF16(search_terms));
    history::BlockUntilHistoryProcessesPendingRequests(
        client_->GetHistoryService());
  }

  // AutocompleteProviderListener:
  void OnProviderUpdate(bool updated_matches,
                        const AutocompleteProvider* provider) override {
    if (autocomplete_->done())
      run_loop_.QuitWhenIdle();
  }

 protected:
  // testing::Test
  void SetUp() override {
    client_ = std::make_unique<FakeAutocompleteProviderClient>();
    auto* registry = static_cast<sync_preferences::TestingPrefServiceSyncable*>(
                         client_->GetPrefs())
                         ->registry();
    omnibox::RegisterBraveProfilePrefs(registry);

    CHECK(history_dir_.CreateUniqueTempDir());
    client_->set_history_service(
        history::CreateHistoryService(history_dir_.GetPath(), true));

    autocomplete_ = base::WrapRefCounted(
        BraveLocalHistoryZeroSuggestProvider::Create(client_.get(), this));

    // Ensure the default search provider is Google, because it's the only
    // provider the LocalHistoryZeroSuggestProvider works with.
    auto url = TemplateURLDataFromPrepopulatedEngine(
        TemplateURLPrepopulateData::google);
    auto* search_provider = client_->GetTemplateURLService()->Add(
        std::make_unique<TemplateURL>(*url));
    client_->GetTemplateURLService()->SetUserSelectedDefaultSearchProvider(
        search_provider);

    // Verify that Google is the default search provider.
    ASSERT_EQ(SEARCH_ENGINE_GOOGLE,
              default_search_provider()->GetEngineType(
                  client_->GetTemplateURLService()->search_terms_data()));

    AddSearchTerm("hello world", 10);
  }

  void TearDown() override {
    autocomplete_ = nullptr;
    client_.reset();
  }

  const TemplateURL* default_search_provider() {
    return client_->GetTemplateURLService()->GetDefaultSearchProvider();
  }

  base::test::TaskEnvironment task_environment_;

  base::RunLoop run_loop_;
  base::ScopedTempDir history_dir_;
  ACMatches matches_;
  std::unique_ptr<FakeAutocompleteProviderClient> client_;
  scoped_refptr<BraveLocalHistoryZeroSuggestProvider> autocomplete_;
};

TEST_F(BraveLocalHistoryZeroSuggestProviderTest, NoResultsWhenHistoryDisabled) {
  client_->GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, false);

  autocomplete_->Start(CreateAutocompleteInput(""), false);
  if (!autocomplete_->done())
    run_loop_.Run();
  EXPECT_TRUE(autocomplete_->matches().empty());
}

TEST_F(BraveLocalHistoryZeroSuggestProviderTest,
       ResultsWhenHistorySuggestionsEnabled) {
  client_->GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, true);

  autocomplete_->Start(CreateAutocompleteInput(""), false);
  if (!autocomplete_->done())
    run_loop_.Run();
  EXPECT_FALSE(autocomplete_->matches().empty());
}
