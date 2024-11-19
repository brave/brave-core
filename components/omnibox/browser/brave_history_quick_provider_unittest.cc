/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/omnibox/browser/brave_history_quick_provider.h"

#include <stddef.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "brave/components/omnibox/browser/brave_omnibox_prefs.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/history/core/browser/history_backend.h"
#include "components/history/core/browser/history_database.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/omnibox/browser/fake_autocomplete_provider_client.h"
#include "components/omnibox/browser/history_test_util.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Post history_backend->GetURL() to the history thread and stop the originating
// thread's message loop when done.
class GetURLTask : public history::HistoryDBTask {
 public:
  GetURLTask(const GURL& url, bool* result_storage)
      : result_storage_(result_storage), url_(url) {}
  GetURLTask(const GetURLTask&) = delete;
  GetURLTask& operator=(const GetURLTask&) = delete;

  bool RunOnDBThread(history::HistoryBackend* backend,
                     history::HistoryDatabase* db) override {
    *result_storage_ = backend->GetURL(url_, nullptr);
    return true;
  }

  void DoneRunOnMainThread() override { base::RunLoop().RunUntilIdle(); }

 private:
  ~GetURLTask() override = default;

  raw_ptr<bool> result_storage_;
  const GURL url_;
};

}  // namespace

class BraveHistoryQuickProviderTest : public testing::Test {
 public:
  BraveHistoryQuickProviderTest() = default;
  BraveHistoryQuickProviderTest(const BraveHistoryQuickProviderTest&) = delete;
  BraveHistoryQuickProviderTest& operator=(
      const BraveHistoryQuickProviderTest&) = delete;

 protected:
  struct TestURLInfo {
    std::string url;
    std::string title;
    int visit_count;
    int typed_count;
    int days_from_now;
  };

  class SetShouldContain {
   public:
    explicit SetShouldContain(const ACMatches& matched_urls);

    void operator()(const std::string& expected);

    std::set<std::string> LeftOvers() const { return matches_; }

   private:
    std::set<std::string> matches_;
  };

  void SetUp() override {
    client_ = std::make_unique<FakeAutocompleteProviderClient>();
    auto* registry = static_cast<sync_preferences::TestingPrefServiceSyncable*>(
                         client_->GetPrefs())
                         ->registry();
    omnibox::RegisterBraveProfilePrefs(registry);
    CHECK(history_dir_.CreateUniqueTempDir());

    client_->set_history_service(
        history::CreateHistoryService(history_dir_.GetPath(), true));
    ASSERT_NE(client_->GetHistoryService(), nullptr);
    ASSERT_NO_FATAL_FAILURE(FillData());

    client_->set_bookmark_model(bookmarks::TestBookmarkClient::CreateModel());

    client_->set_in_memory_url_index(std::make_unique<InMemoryURLIndex>(
        client_->GetBookmarkModel(), client_->GetHistoryService(), nullptr,
        history_dir_.GetPath(), SchemeSet()));
    client_->GetInMemoryURLIndex()->Init();
    // Block until History has processed InMemoryURLIndex initialization.
    history::BlockUntilHistoryProcessesPendingRequests(
        client_->GetHistoryService());
    ASSERT_TRUE(client_->GetInMemoryURLIndex()->restored());

    provider_ = base::MakeRefCounted<BraveHistoryQuickProvider>(client_.get());
  }

  void TearDown() override {
    ac_matches_.clear();
    provider_ = nullptr;
    client_.reset();
  }

  // Fills test data into the history system.
  void FillData() {
    history::URLRow row{GURL("http://example.com/")};
    ASSERT_TRUE(row.url().is_valid());
    row.set_title(u"Example");
    row.set_visit_count(3);
    row.set_typed_count(3);
    row.set_last_visit(base::Time::Now());

    AddFakeURLToHistoryDB(history_backend()->db(), row);
  }

  // Runs an autocomplete query on |text| and checks to see that the returned
  // results' destination URLs match those provided. |expected_urls| does not
  // need to be in sorted order.
  void RunTest(const std::u16string& text,
               bool prevent_inline_autocomplete,
               const std::vector<std::string>& expected_urls,
               bool expected_can_inline_top_result,
               const std::u16string& expected_fill_into_edit,
               const std::u16string& expected_autocompletion) {
    RunTestWithCursor(text, std::u16string::npos, prevent_inline_autocomplete,
                      expected_urls, expected_can_inline_top_result,
                      expected_fill_into_edit, expected_autocompletion);
  }

  // As above, simply with a cursor position specified.
  void RunTestWithCursor(const std::u16string& text,
                         const size_t cursor_position,
                         bool prevent_inline_autocomplete,
                         const std::vector<std::string>& expected_urls,
                         bool expected_can_inline_top_result,
                         const std::u16string& expected_fill_into_edit,
                         const std::u16string& expected_autocompletion,
                         bool duplicates_ok = false) {
    SCOPED_TRACE(text);  // Minimal hint to query being run.
    base::RunLoop().RunUntilIdle();
    AutocompleteInput input(text, cursor_position,
                            metrics::OmniboxEventProto::OTHER,
                            TestSchemeClassifier());
    input.set_prevent_inline_autocomplete(prevent_inline_autocomplete);
    provider_->Start(input, false);
    EXPECT_TRUE(provider_->done());

    ac_matches_ = provider_->matches();

    // We should have gotten back at most
    // AutocompleteProvider::provider_max_matches().
    EXPECT_LE(ac_matches_.size(), provider_->provider_max_matches());

    // If the number of expected and actual matches aren't equal then we need
    // test no further, but let's do anyway so that we know which URLs failed.
    if (duplicates_ok)
      EXPECT_LE(expected_urls.size(), ac_matches_.size());
    else
      EXPECT_EQ(expected_urls.size(), ac_matches_.size());

    // Verify that all expected URLs were found and that all found URLs
    // were expected.
    std::set<std::string> leftovers;
    if (duplicates_ok) {
      for (const auto& match : ac_matches_)
        leftovers.insert(match.destination_url.spec());
      for (const auto& expected : expected_urls)
        EXPECT_EQ(1U, leftovers.count(expected)) << "Expected URL " << expected;
      for (const auto& expected : expected_urls)
        leftovers.erase(expected);
    } else {
      leftovers = for_each(expected_urls.begin(), expected_urls.end(),
                           SetShouldContain(ac_matches_))
                      .LeftOvers();
    }
    EXPECT_EQ(0U, leftovers.size())
        << "There were " << leftovers.size()
        << " unexpected results, one of which was: '" << *(leftovers.begin())
        << "'.";

    if (expected_urls.empty())
      return;

    ASSERT_FALSE(ac_matches_.empty());
    // Verify that we got the results in the order expected.
    int best_score = ac_matches_.begin()->relevance + 1;
    int i = 0;
    std::vector<std::string>::const_iterator expected = expected_urls.begin();
    for (ACMatches::const_iterator actual = ac_matches_.begin();
         actual != ac_matches_.end() && expected != expected_urls.end();
         ++actual, ++expected, ++i) {
      EXPECT_EQ(*expected, actual->destination_url.spec())
          << "For result #" << i << " we got '"
          << actual->destination_url.spec() << "' but expected '" << *expected
          << "'.";
      EXPECT_LT(actual->relevance, best_score)
          << "At result #" << i << " (url=" << actual->destination_url.spec()
          << "), we noticed scores are not monotonically decreasing.";
      best_score = actual->relevance;
    }

    EXPECT_EQ(expected_can_inline_top_result,
              ac_matches_[0].allowed_to_be_default_match);
    if (expected_can_inline_top_result)
      EXPECT_EQ(expected_autocompletion, ac_matches_[0].inline_autocompletion);
    EXPECT_EQ(expected_fill_into_edit, ac_matches_[0].fill_into_edit);
  }

  // TODO(shess): From history_service.h in reference to history_backend:
  // > This class has most of the implementation and runs on the 'thread_'.
  // > You MUST communicate with this class ONLY through the thread_'s
  // > message_loop().
  // Direct use of this object in tests is almost certainly not thread-safe.
  history::HistoryBackend* history_backend() {
    return client_->GetHistoryService()->history_backend_.get();
  }

  FakeAutocompleteProviderClient& client() { return *client_; }
  ACMatches& ac_matches() { return ac_matches_; }
  HistoryQuickProvider& provider() { return *provider_; }

 private:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir history_dir_;
  std::unique_ptr<FakeAutocompleteProviderClient> client_;

  scoped_refptr<BraveHistoryQuickProvider> provider_;
  ACMatches ac_matches_;  // The resulting matches after running RunTest.
};

BraveHistoryQuickProviderTest::SetShouldContain::SetShouldContain(
    const ACMatches& matched_urls) {
  for (const auto& iter : matched_urls)
    matches_.insert(iter.destination_url.spec());
}

void BraveHistoryQuickProviderTest::SetShouldContain::operator()(
    const std::string& expected) {
  EXPECT_EQ(1U, matches_.erase(expected))
      << "Results did not contain '" << expected << "' but should have.";
}

TEST_F(BraveHistoryQuickProviderTest, HasResultsWhenHistoryEnabled) {
  client().GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, true);
  std::vector<std::string> expected_urls;
  expected_urls.push_back("http://example.com/");
  // With cursor after "slash", we should retrieve the desired result but it
  // should not be allowed to be the default match.
  RunTest(u"example", false, expected_urls, true, u"example.com", u".com");
}

TEST_F(BraveHistoryQuickProviderTest, HasNoResultsWhenHistoryDisabled) {
  client().GetPrefs()->SetBoolean(omnibox::kHistorySuggestionsEnabled, false);
  std::vector<std::string> expected_urls;
  // With cursor after "slash", we should retrieve the desired result but it
  // should not be allowed to be the default match.
  RunTest(u"example", false, expected_urls, true, u"example.com", u".com");
}
