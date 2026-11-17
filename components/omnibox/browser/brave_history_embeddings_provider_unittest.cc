// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Based on Chromium code subject to the following license:
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/components/omnibox/browser/brave_history_embeddings_provider.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "brave/components/history_embeddings/core/history_embeddings_search.h"
#include "brave/components/history_embeddings/core/vector_database.h"
#include "brave/components/omnibox/browser/brave_fake_autocomplete_provider_client.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/url_row.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/omnibox/browser/autocomplete_enums.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/autocomplete_provider_listener.h"
#include "components/omnibox/browser/test_scheme_classifier.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/omnibox_event.pb.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace {

AutocompleteInput CreateAutocompleteInput(const std::u16string& input) {
  return {input, metrics::OmniboxEventProto::OTHER, TestSchemeClassifier()};
}

brave_history_embeddings::ScoredUrlRow CreateScoredUrlRow(
    float score,
    const std::string& url,
    const std::u16string& title) {
  brave_history_embeddings::ScoredUrlRow scored_url_row(
      brave_history_embeddings::ScoredUrl(0, 0, {}, score, 0));
  scored_url_row.row = history::URLRow{GURL{url}};
  scored_url_row.row.set_title(title);
  scored_url_row.url_data.passages.add_passages("passage");
  std::vector<float> data(768, 0.0f);
  data[0] = 1.0f;
  scored_url_row.url_data.passage_embeddings.emplace_back(
      brave_history_embeddings::PassageEmbedding(
          passage_embeddings::Embedding(std::move(data)), 1));

  scored_url_row.scores.push_back(score);
  return scored_url_row;
}

brave_history_embeddings::SearchResult CreateSearchResult(
    const std::string& query,
    const std::u16string& title) {
  brave_history_embeddings::SearchResult result;
  result.query = query;
  result.scored_url_rows = {
      CreateScoredUrlRow(.5, "https://url.com/", title),
  };
  return result;
}

TemplateURL CreateTemplateUrl() {
  TemplateURLData template_url_data;
  template_url_data.SetShortName(u"shortname");
  template_url_data.SetKeyword(u"keyword");
  template_url_data.SetURL("https://url.com");
  return TemplateURL{template_url_data};
}

class MockHistoryEmbeddingsSearch
    : public brave_history_embeddings::HistoryEmbeddingsSearch {
 public:
  MOCK_METHOD(brave_history_embeddings::SearchResult,
              Search,
              (brave_history_embeddings::SearchResult * previous_search_result,
               std::string query,
               std::optional<base::Time> time_range_start,
               size_t count,
               std::vector<history::URLID> url_id_filter,
               brave_history_embeddings::SearchResultCallback callback),
              (override));
};

// Hands the provider a search, as the real client does once the profile's
// service was built.
class HistoryEmbeddingsAutocompleteProviderClient
    : public BraveFakeAutocompleteProviderClient {
 public:
  brave_history_embeddings::HistoryEmbeddingsSearch*
  GetBraveHistoryEmbeddingsSearch() override {
    return search_.get();
  }

  history::HistoryService* GetHistoryService() override {
    return history_service_.get();
  }

  void set_history_service(std::unique_ptr<history::HistoryService> service) {
    history_service_ = std::move(service);
  }

  void set_search(
      std::unique_ptr<brave_history_embeddings::HistoryEmbeddingsSearch>
          search) {
    search_ = std::move(search);
  }

 private:
  std::unique_ptr<history::HistoryService> history_service_;
  std::unique_ptr<brave_history_embeddings::HistoryEmbeddingsSearch> search_;
};

}  // namespace

class FakeBraveHistoryEmbeddingsProvider
    : public BraveHistoryEmbeddingsProvider {
 public:
  using BraveHistoryEmbeddingsProvider::BraveHistoryEmbeddingsProvider;
  using BraveHistoryEmbeddingsProvider::OnReceivedSearchResult;

  using BraveHistoryEmbeddingsProvider::done_;
  using BraveHistoryEmbeddingsProvider::input_;
  using BraveHistoryEmbeddingsProvider::matches_;
  using BraveHistoryEmbeddingsProvider::starter_pack_engine_;

 private:
  ~FakeBraveHistoryEmbeddingsProvider() override = default;
};

class BraveHistoryEmbeddingsProviderTest : public testing::Test,
                                           public AutocompleteProviderListener {
 public:
  void SetUp() override {
    testing::Test::SetUp();

    CHECK(history_dir_.CreateUniqueTempDir());
    client_ = std::make_unique<HistoryEmbeddingsAutocompleteProviderClient>();
    client_->set_history_service(
        history::CreateHistoryService(history_dir_.GetPath(), true));

    auto mock_search =
        std::make_unique<testing::NiceMock<MockHistoryEmbeddingsSearch>>();
    history_embeddings_search_ = mock_search.get();
    client_->set_search(std::move(mock_search));

    history_embeddings_provider_ =
        new FakeBraveHistoryEmbeddingsProvider(client_.get(), this);

    // When `Search()` is called, pushes a callback to `search_callbacks_` that
    // can be ran to simulate `Search()` responding asyncly.
    ON_CALL(*history_embeddings_search_,
            Search(testing::_, testing::_, testing::_, testing::_, testing::_,
                   testing::_))
        .WillByDefault(
            [&](brave_history_embeddings::SearchResult* previous_search_result,
                std::string query, std::optional<base::Time> time_range_start,
                size_t count, std::vector<history::URLID> url_id_filter,
                brave_history_embeddings::SearchResultCallback callback) {
              search_callbacks_.push_back(base::BindOnce(
                  [](brave_history_embeddings::SearchResultCallback callback,
                     std::string query, std::u16string response) {
                    std::move(callback).Run(
                        CreateSearchResult(query, response));
                  },
                  std::move(callback)));
              return brave_history_embeddings::SearchResult();
            });
  }

  void TearDown() override {
    history_embeddings_search_ = nullptr;
    history_embeddings_provider_ = nullptr;
    client_.reset();
    brave_history_embeddings::SetFeatureParametersForTesting(
        brave_history_embeddings::FeatureParameters(/*load_finch=*/false));
    testing::Test::TearDown();
  }

  // AutocompleteProviderListener:
  void OnProviderUpdate(bool updated_matches,
                        const AutocompleteProvider* provider) override {
    last_update_matches_ = history_embeddings_provider_->matches_;
  }

 protected:
  void SetSearchQueryMinimumWordCount(size_t count) {
    auto feature_parameters = brave_history_embeddings::GetFeatureParameters();
    feature_parameters.search_query_minimum_word_count = count;
    brave_history_embeddings::SetFeatureParametersForTesting(
        feature_parameters);
  }

  base::ScopedTempDir history_dir_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<HistoryEmbeddingsAutocompleteProviderClient> client_;
  raw_ptr<testing::NiceMock<MockHistoryEmbeddingsSearch>>
      history_embeddings_search_;
  scoped_refptr<FakeBraveHistoryEmbeddingsProvider>
      history_embeddings_provider_;
  // Callbacks created when `Search()` is called. Running a callback with
  // `(query, response)` will simulate `Search()` responding with a result with
  // query `query` and 1 row  with title `response`.
  std::vector<base::OnceCallback<void(std::string, std::u16string)>>
      search_callbacks_;
  // The last set of matches the provider gave the autocomplete controller.
  ACMatches last_update_matches_;
};

TEST_F(BraveHistoryEmbeddingsProviderTest, Start) {
  AutocompleteInput short_input = CreateAutocompleteInput(u"query");
  AutocompleteInput sync_long_input =
      CreateAutocompleteInput(u"query query query");
  sync_long_input.set_omit_asynchronous_matches(true);
  AutocompleteInput long_input = CreateAutocompleteInput(u"query query query");

  // Short queries should be blocked.
  SetSearchQueryMinimumWordCount(3);
  EXPECT_CALL(*history_embeddings_search_,
              Search(testing::_, testing::_, testing::_, testing::_, testing::_,
                     testing::_))
      .Times(0);
  history_embeddings_provider_->Start(short_input, false);

  // Sync queries should be blocked.
  EXPECT_CALL(*history_embeddings_search_,
              Search(testing::_, testing::_, testing::_, testing::_, testing::_,
                     testing::_))
      .Times(0);
  history_embeddings_provider_->Start(sync_long_input, false);

  // Long queries should pass.
  EXPECT_CALL(*history_embeddings_search_,
              Search(testing::_, "query query query",
                     std::optional<base::Time>{}, 3u, testing::_, testing::_))
      .Times(1);
  history_embeddings_provider_->Start(long_input, false);
}

// Without a service for the profile, the provider has nothing to search.
TEST_F(BraveHistoryEmbeddingsProviderTest, Start_NoSearch) {
  // Drop the raw_ptr before the client frees what it points at.
  history_embeddings_search_ = nullptr;
  client_->set_search(nullptr);

  history_embeddings_provider_->Start(
      CreateAutocompleteInput(u"query query query"), false);
  EXPECT_TRUE(history_embeddings_provider_->done_);
  EXPECT_TRUE(history_embeddings_provider_->matches_.empty());
}

TEST_F(BraveHistoryEmbeddingsProviderTest, Start_MultipleSequentialSearches) {
  // Start 1st search.
  history_embeddings_provider_->Start(CreateAutocompleteInput(u"1 1 1"), false);
  EXPECT_TRUE(last_update_matches_.empty());

  // Check results are populated when 1st search responds.
  std::move(search_callbacks_[0]).Run("1 1 1", u"1");
  EXPECT_THAT(last_update_matches_,
              testing::ElementsAre(
                  testing::Field(&AutocompleteMatch::description, u"1")));

  // Start 2nd search.
  history_embeddings_provider_->Start(CreateAutocompleteInput(u"2 2 2"), false);
  EXPECT_THAT(last_update_matches_,
              testing::ElementsAre(
                  testing::Field(&AutocompleteMatch::description, u"1")));

  // Check results are populated when 2nd search responds.
  std::move(search_callbacks_[1]).Run("2 2 2", u"2");
  EXPECT_THAT(last_update_matches_,
              testing::ElementsAre(
                  testing::Field(&AutocompleteMatch::description, u"2")));
}

TEST_F(BraveHistoryEmbeddingsProviderTest, Start_MultipleParallelSearches) {
  // Start 1st search.
  history_embeddings_provider_->Start(CreateAutocompleteInput(u"1 1 1"), false);
  EXPECT_TRUE(last_update_matches_.empty());

  // Start 2nd search.
  history_embeddings_provider_->Start(CreateAutocompleteInput(u"2 2 2"), false);
  EXPECT_TRUE(last_update_matches_.empty());

  // Check results are not populated when 1st search responds.
  std::move(search_callbacks_[0]).Run("1 1 1", u"1");
  EXPECT_TRUE(last_update_matches_.empty());

  // Check results are populated when 2nd search responds.
  std::move(search_callbacks_[1]).Run("2 2 2", u"2");
  EXPECT_THAT(last_update_matches_,
              testing::ElementsAre(
                  testing::Field(&AutocompleteMatch::description, u"2")));
}

TEST_F(BraveHistoryEmbeddingsProviderTest,
       Start_MultipleParallelSearchesWithSameQuery) {
  // Start 1st search.
  history_embeddings_provider_->Start(CreateAutocompleteInput(u"1 1 1"), false);
  EXPECT_TRUE(last_update_matches_.empty());

  // Start 2nd search.
  history_embeddings_provider_->Start(CreateAutocompleteInput(u"1 1 1"), false);
  EXPECT_TRUE(last_update_matches_.empty());

  // Check results are populated when 1st search responds. Even though the
  // provider usually only cares about the most recent `Search()`, since the
  // input didn't change, it can use the 1st `Search()`.
  std::move(search_callbacks_[0]).Run("1 1 1", u"1");
  EXPECT_THAT(last_update_matches_,
              testing::ElementsAre(
                  testing::Field(&AutocompleteMatch::description, u"1")));

  // Check results aren't replaced when 2nd search responds. The provider
  // already reported `done_ = true` and it would break autocompletion to send
  // an update after doing so.
  std::move(search_callbacks_[1]).Run("1 1 1", u"2");
  EXPECT_THAT(last_update_matches_,
              testing::ElementsAre(
                  testing::Field(&AutocompleteMatch::description, u"1")));
}

TEST_F(BraveHistoryEmbeddingsProviderTest,
       Start_MultipleParallelSearchesWithIneligibleQuery) {
  SetSearchQueryMinimumWordCount(3);

  // Start 1st search.
  history_embeddings_provider_->Start(CreateAutocompleteInput(u"1 1 1"), false);
  EXPECT_TRUE(last_update_matches_.empty());

  // Start 2nd search. It's too short.
  history_embeddings_provider_->Start(CreateAutocompleteInput(u"2 2"), false);
  EXPECT_TRUE(last_update_matches_.empty());

  // Ensure a stale search doesn't populate matches.
  std::move(search_callbacks_[0]).Run("1 1 1", u"1");
  EXPECT_TRUE(last_update_matches_.empty());

  // Ensure a 2nd search wasn't made.
  EXPECT_EQ(search_callbacks_.size(), 1u);
}

TEST_F(BraveHistoryEmbeddingsProviderTest,
       Start_Stop_SearchCompletesAfterStop) {
  // Start search.
  history_embeddings_provider_->Start(CreateAutocompleteInput(u"1 1 1"), false);
  EXPECT_TRUE(last_update_matches_.empty());

  history_embeddings_provider_->Stop(AutocompleteStopReason::kClobbered);

  // Results returned after `Stop()` should be discarded.
  std::move(search_callbacks_[0]).Run("1 1 1", u"1");
  EXPECT_TRUE(last_update_matches_.empty());
}

// A search that outlasts the stop timer still delivers its results.
TEST_F(BraveHistoryEmbeddingsProviderTest,
       Start_StopTimer_SearchCompletesAfterStop) {
  history_embeddings_provider_->Start(CreateAutocompleteInput(u"1 1 1"), false);

  history_embeddings_provider_->Stop(AutocompleteStopReason::kInactivity);

  std::move(search_callbacks_[0]).Run("1 1 1", u"1");
  EXPECT_THAT(last_update_matches_,
              testing::ElementsAre(
                  testing::Field(&AutocompleteMatch::description, u"1")));
}

TEST_F(BraveHistoryEmbeddingsProviderTest, Stop) {
  history_embeddings_provider_->done_ = false;

  history_embeddings_provider_->Stop(AutocompleteStopReason::kInactivity);
  EXPECT_FALSE(history_embeddings_provider_->done_);

  history_embeddings_provider_->Stop(AutocompleteStopReason::kClobbered);
  EXPECT_TRUE(history_embeddings_provider_->done_);
}

TEST_F(BraveHistoryEmbeddingsProviderTest, DeleteMatch) {
  AutocompleteMatch match(history_embeddings_provider_.get(), 1000, true,
                          omnibox::AutocompleteMatchType::kHistoryEmbeddings);
  match.destination_url = GURL{"https://en.wikipedia.org/wiki/Matenadaran"};
  history_embeddings_provider_->matches_.push_back(match);
  history_embeddings_provider_->DeleteMatch(match);
  EXPECT_TRUE(history_embeddings_provider_->matches_.empty());
}

TEST_F(BraveHistoryEmbeddingsProviderTest,
       OnReceivedSearchResult_CreatesAutocompleteMatches) {
  brave_history_embeddings::SearchResult result;
  result.query = "query";
  result.scored_url_rows = {
      CreateScoredUrlRow(.5, "https://url.com/", u"title"),
  };
  history_embeddings_provider_->done_ = false;
  history_embeddings_provider_->input_ = CreateAutocompleteInput(u"query");
  history_embeddings_provider_->OnReceivedSearchResult(std::move(result));

  EXPECT_TRUE(history_embeddings_provider_->done_);
  ASSERT_EQ(history_embeddings_provider_->matches_.size(), 1u);
  EXPECT_EQ(history_embeddings_provider_->matches_[0].provider.get(),
            history_embeddings_provider_.get());
  EXPECT_EQ(history_embeddings_provider_->matches_[0].relevance, 500);
  EXPECT_EQ(history_embeddings_provider_->matches_[0].deletable, true);
  EXPECT_EQ(history_embeddings_provider_->matches_[0].type,
            omnibox::AutocompleteMatchType::kHistoryEmbeddings);
  EXPECT_EQ(history_embeddings_provider_->matches_[0].destination_url.spec(),
            "https://url.com/");
  EXPECT_EQ(history_embeddings_provider_->matches_[0].description, u"title");
  EXPECT_EQ(history_embeddings_provider_->matches_[0].contents,
            u"https://url.com/");
  EXPECT_EQ(history_embeddings_provider_->matches_[0].keyword, u"");
  EXPECT_TRUE(PageTransitionCoreTypeIs(
      history_embeddings_provider_->matches_[0].transition,
      ui::PAGE_TRANSITION_TYPED));
}

TEST_F(BraveHistoryEmbeddingsProviderTest,
       OnReceivedSearchResult_CreatesScopedAutocompleteMatches) {
  // Verifies autocomplete match is created correctly when the user is in
  // keyword mode.
  TemplateURL template_url = CreateTemplateUrl();
  history_embeddings_provider_->starter_pack_engine_ = &template_url;

  brave_history_embeddings::SearchResult result;
  result.query = "query";
  result.scored_url_rows = {
      CreateScoredUrlRow(.5, "https://url.com/", u"title"),
  };
  history_embeddings_provider_->done_ = false;
  history_embeddings_provider_->input_ = CreateAutocompleteInput(u"query");
  history_embeddings_provider_->OnReceivedSearchResult(std::move(result));

  EXPECT_TRUE(history_embeddings_provider_->done_);
  ASSERT_EQ(history_embeddings_provider_->matches_.size(), 1u);
  EXPECT_EQ(history_embeddings_provider_->matches_[0].provider.get(),
            history_embeddings_provider_.get());
  EXPECT_EQ(history_embeddings_provider_->matches_[0].relevance, 500);
  EXPECT_EQ(history_embeddings_provider_->matches_[0].deletable, true);
  EXPECT_EQ(history_embeddings_provider_->matches_[0].type,
            omnibox::AutocompleteMatchType::kHistoryEmbeddings);
  EXPECT_EQ(history_embeddings_provider_->matches_[0].destination_url.spec(),
            "https://url.com/");
  EXPECT_EQ(history_embeddings_provider_->matches_[0].description, u"title");
  EXPECT_EQ(history_embeddings_provider_->matches_[0].contents,
            u"https://url.com/");
  EXPECT_EQ(history_embeddings_provider_->matches_[0].keyword, u"keyword");
  EXPECT_TRUE(PageTransitionCoreTypeIs(
      history_embeddings_provider_->matches_[0].transition,
      ui::PAGE_TRANSITION_KEYWORD));
  history_embeddings_provider_->starter_pack_engine_ = nullptr;
}
