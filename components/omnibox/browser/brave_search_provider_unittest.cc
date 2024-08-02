/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stddef.h>

#include <algorithm>
#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "brave/components/omnibox/browser/brave_search_provider.h"
#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/autocomplete/chrome_autocomplete_provider_client.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider.h"
#include "components/omnibox/browser/search_provider.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"

// BraveSearchProviderTest -----------------------------------------------------

class BraveSearchProviderTest : public testing::Test {
 public:
  BraveSearchProviderTest() {
    TestingProfile::Builder profile_builder;
    profile_builder.AddTestingFactory(
        HistoryServiceFactory::GetInstance(),
        HistoryServiceFactory::GetDefaultFactory());
    profile_builder.AddTestingFactory(
        TemplateURLServiceFactory::GetInstance(),
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));

    profile_ = profile_builder.Build();
  }

  BraveSearchProviderTest(const BraveSearchProviderTest&) = delete;
  BraveSearchProviderTest& operator=(const BraveSearchProviderTest&) = delete;

  void SetUp() override {
    std::string search_url = "http://defaultturl/{searchTerms}";
    std::string suggestions_url = "http://defaultturl2/{searchTerms}";
    TemplateURLService* turl_model =
        TemplateURLServiceFactory::GetForProfile(profile_.get());

    turl_model->Load();

    // Reset the default TemplateURL.
    TemplateURLData data;
    data.SetShortName(u"t");
    data.SetURL(search_url);
    data.suggestions_url = suggestions_url;
    default_t_url_ = turl_model->Add(std::make_unique<TemplateURL>(data));
    turl_model->SetUserSelectedDefaultSearchProvider(default_t_url_);
    TemplateURLID default_provider_id = default_t_url_->id();
    ASSERT_NE(0, default_provider_id);

    // Keywords are updated by the InMemoryHistoryBackend only after the message
    // has been processed on the history thread. Block until history processes
    // all requests to ensure the InMemoryDatabase is the state we expect it.
    profile_->BlockUntilHistoryProcessesPendingRequests();

    AutocompleteClassifierFactory::GetInstance()->SetTestingFactoryAndUse(
        profile_.get(),
        base::BindRepeating(&AutocompleteClassifierFactory::BuildInstanceFor));

    client_ =
        std::make_unique<ChromeAutocompleteProviderClient>(profile_.get());
    provider_ =
        base::MakeRefCounted<BraveSearchProvider>(client_.get(), nullptr);
  }

  void TearDown() override {
    // Shutdown the provider before the profile.
    provider_ = nullptr;
  }

 protected:
  // Adds a search for |term|, using the engine |t_url| to the history, and
  // returns the URL for that search.
  GURL AddSearchToHistory(TemplateURL* t_url,
                          std::u16string term,
                          int visit_count) {
    history::HistoryService* history = HistoryServiceFactory::GetForProfile(
        profile_.get(), ServiceAccessType::EXPLICIT_ACCESS);
    GURL search(t_url->url_ref().ReplaceSearchTerms(
        TemplateURLRef::SearchTermsArgs(term),
        TemplateURLServiceFactory::GetForProfile(profile_.get())
            ->search_terms_data()));
    last_added_time_ =
        std::max(base::Time::Now(), last_added_time_ + base::Microseconds(1));
    history->AddPageWithDetails(search, std::u16string(), visit_count,
                                visit_count, last_added_time_, false,
                                history::SOURCE_BROWSED);
    history->SetKeywordSearchTermsForURL(search, t_url->id(), term);
    return search;
  }

  // Looks for a match in |provider_| with destination |url|.  Sets |match| to
  // it if found.  Returns whether |match| was set.
  bool FindMatchWithDestination(const GURL& url, AutocompleteMatch* match) {
    for (const auto& i : provider_->matches()) {
      if (i.destination_url == url) {
        *match = i;
        return true;
      }
    }
    return false;
  }

  void QueryForInputAndSetWYTMatch(const std::u16string& text,
                                   AutocompleteMatch* wyt_match) {
    AutocompleteInput input(text, metrics::OmniboxEventProto::OTHER,
                            ChromeAutocompleteSchemeClassifier(profile_.get()));
    provider_->Start(input, false);

    // RunUntilIdle so that the task scheduled by SearchProvider to create the
    // URLFetchers runs.
    base::RunLoop().RunUntilIdle();

    profile_->BlockUntilHistoryProcessesPendingRequests();
    DCHECK(wyt_match);
    ASSERT_GE(provider_->matches().size(), 1u);
    EXPECT_TRUE(FindMatchWithDestination(
        GURL(default_t_url_->url_ref().ReplaceSearchTerms(
            TemplateURLRef::SearchTermsArgs(
                base::CollapseWhitespace(text, false)),
            TemplateURLServiceFactory::GetForProfile(profile_.get())
                ->search_terms_data())),
        wyt_match));
  }

  base::Time last_added_time_;

  content::BrowserTaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<ChromeAutocompleteProviderClient> client_;
  scoped_refptr<BraveSearchProvider> provider_;

  raw_ptr<TemplateURL> default_t_url_ = nullptr;

  // If not nullptr, OnProviderUpdate quits the current |run_loop_|.
  raw_ptr<base::RunLoop> run_loop_ = nullptr;
};

// Actual Tests
// ---------------------------------------------------------------

TEST_F(BraveSearchProviderTest, SearchIncludesHistoryWhenHistoryEnabled) {
  profile_->GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, true);

  GURL term_url_a(AddSearchToHistory(default_t_url_, u"hello", 1));
  profile_->BlockUntilHistoryProcessesPendingRequests();

  AutocompleteMatch wyt_match;
  ASSERT_NO_FATAL_FAILURE(QueryForInputAndSetWYTMatch(u"hel", &wyt_match));
  ASSERT_EQ(2u, provider_->matches().size());
}

TEST_F(BraveSearchProviderTest,
       SearchDoesNotIncludeHistoryWhenHistoryDisabled) {
  profile_->GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, false);

  GURL term_url_a(AddSearchToHistory(default_t_url_, u"hello", 1));
  profile_->BlockUntilHistoryProcessesPendingRequests();

  AutocompleteMatch wyt_match;
  ASSERT_NO_FATAL_FAILURE(QueryForInputAndSetWYTMatch(u"hel", &wyt_match));
  ASSERT_EQ(1u, provider_->matches().size());
}
