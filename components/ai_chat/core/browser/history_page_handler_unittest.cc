// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/history_page_handler.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/history.mojom.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/test/test_history_database.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

class HistoryPageHandlerTest : public testing::Test {
 public:
  HistoryPageHandlerTest() = default;
  ~HistoryPageHandlerTest() override = default;

  void SetUp() override {
    // Create temp directory for history database
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    history_dir_ = temp_dir_.GetPath().AppendASCII("HistoryTest");
    ASSERT_TRUE(base::CreateDirectory(history_dir_));

    // Create HistoryService - runs synchronously in tests
    history_service_ = std::make_unique<history::HistoryService>();
    if (!history_service_->Init(
            history::TestHistoryDatabaseParamsForPath(history_dir_))) {
      history_service_.reset();
      ADD_FAILURE();
    }

    // Create HistoryPageHandler
    mojo::PendingReceiver<mojom::HistoryPageHandler> receiver;
    history_page_handler_ = std::make_unique<HistoryPageHandler>(
        std::move(receiver), history_service_.get());
  }

  void TearDown() override { history_service_->Shutdown(); }

  void AddTestHistoryEntry(const std::string& title, const GURL& url) {
    history_service_->AddPage(url, base::Time::Now(), history::SOURCE_BROWSED);
    history_service_->SetPageTitle(url, base::UTF8ToUTF16(title));
  }

  std::vector<mojom::HistoryEntryPtr> GetHistory(
      const std::optional<std::string>& query = std::nullopt,
      std::optional<uint32_t> max_results = std::nullopt) {
    base::test::TestFuture<std::vector<mojom::HistoryEntryPtr>> future;
    history_page_handler_->GetHistory(query, max_results, future.GetCallback());
    return future.Take();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  base::FilePath history_dir_;
  std::unique_ptr<history::HistoryService> history_service_;
  std::unique_ptr<HistoryPageHandler> history_page_handler_;
};

TEST_F(HistoryPageHandlerTest, EmptyHistory) {
  // Should return empty list when no history exists
  auto history = GetHistory();
  EXPECT_TRUE(history.empty());
}

TEST_F(HistoryPageHandlerTest, GetMultipleHistoryEntries) {
  // Add multiple history entries
  AddTestHistoryEntry("Example 1", GURL("https://example1.com"));
  AddTestHistoryEntry("Example 2", GURL("https://example2.com"));
  AddTestHistoryEntry("Example 3", GURL("https://example3.com"));

  auto history = GetHistory();
  ASSERT_EQ(3u, history.size());

  // Verify entries are present (order may vary)
  std::vector<std::string> titles;
  std::vector<GURL> urls;
  for (const auto& entry : history) {
    titles.push_back(entry->title);
    urls.push_back(entry->url);
  }

  EXPECT_TRUE(base::Contains(titles, "Example 1"));
  EXPECT_TRUE(base::Contains(titles, "Example 2"));
  EXPECT_TRUE(base::Contains(titles, "Example 3"));
  EXPECT_TRUE(base::Contains(urls, GURL("https://example1.com")));
  EXPECT_TRUE(base::Contains(urls, GURL("https://example2.com")));
  EXPECT_TRUE(base::Contains(urls, GURL("https://example3.com")));
}

TEST_F(HistoryPageHandlerTest, SearchWithQuery) {
  // Add history entries with different titles
  AddTestHistoryEntry("Brave Browser", GURL("https://brave.com"));
  AddTestHistoryEntry("Google Search", GURL("https://google.com"));
  AddTestHistoryEntry("Brave Search", GURL("https://search.brave.com"));

  // Search for "Brave"
  auto history = GetHistory("Brave");

  // Should return only entries matching "Brave"
  ASSERT_EQ(2u, history.size());

  std::vector<std::string> titles;
  for (const auto& entry : history) {
    titles.push_back(entry->title);
  }

  EXPECT_TRUE(base::Contains(titles, "Brave Browser"));
  EXPECT_TRUE(base::Contains(titles, "Brave Search"));
}

TEST_F(HistoryPageHandlerTest, MaxResultsLimit) {
  // Add 10 history entries
  for (int i = 0; i < 10; i++) {
    AddTestHistoryEntry(
        "Entry " + base::NumberToString(i),
        GURL("https://example" + base::NumberToString(i) + ".com"));
  }

  // Request only 5 results
  auto history = GetHistory(std::nullopt, 5);

  EXPECT_EQ(5u, history.size());
}

TEST_F(HistoryPageHandlerTest, DefaultMaxResults) {
  // Add more than default (100) entries
  for (int i = 0; i < 150; i++) {
    AddTestHistoryEntry(
        "Entry " + base::NumberToString(i),
        GURL("https://example" + base::NumberToString(i) + ".com"));
  }

  // Request without specifying max_results (should use default of 100)
  auto history = GetHistory();

  EXPECT_EQ(100u, history.size());
}

TEST_F(HistoryPageHandlerTest, EmptyQuery) {
  // Add history entries
  AddTestHistoryEntry("Entry 1", GURL("https://example1.com"));
  AddTestHistoryEntry("Entry 2", GURL("https://example2.com"));

  // Empty query should return all results
  auto history = GetHistory("");

  EXPECT_EQ(2u, history.size());
}

TEST_F(HistoryPageHandlerTest, URLMatch) {
  // Add history entries
  AddTestHistoryEntry("Entry 1", GURL("https://example1.com"));
  AddTestHistoryEntry("Entry 2", GURL("https://example2.com"));

  // URL match should return just entry 2
  auto history = GetHistory("example2");

  ASSERT_EQ(1u, history.size());
  EXPECT_EQ("Entry 2", history[0]->title);
  EXPECT_EQ(GURL("https://example2.com"), history[0]->url);
}

}  // namespace ai_chat
