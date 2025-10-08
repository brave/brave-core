// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/history_ui_handler.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/location.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/history.mojom.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/test/test_history_database.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

class HistoryUIHandlerTest : public testing::Test {
 public:
  HistoryUIHandlerTest() = default;
  ~HistoryUIHandlerTest() override = default;

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
      ADD_FAILURE() << "Failed to initialize HistoryService";
    }

    // Create HistoryUIHandler
    mojo::PendingReceiver<mojom::HistoryUIHandler> receiver;
    history_ui_handler_ = std::make_unique<HistoryUIHandler>(
        std::move(receiver), history_service_.get());
  }

  void TearDown() override { history_service_->Shutdown(); }

  void AddTestHistoryEntry(const std::string& title, const GURL& url) {
    history_service_->AddPage(url, base::Time::Now() - base::Days(add_count_++),
                              history::SOURCE_BROWSED);
    history_service_->SetPageTitle(url, base::UTF8ToUTF16(title));
  }

  std::vector<mojom::HistoryEntryPtr> GetHistory(
      const std::optional<std::string>& query = std::nullopt,
      std::optional<uint32_t> max_results = std::nullopt) {
    base::test::TestFuture<std::vector<mojom::HistoryEntryPtr>> future;
    history_ui_handler_->GetHistory(query, max_results, future.GetCallback());
    return future.Take();
  }

  void ExpectResults(base::Location location,
                     std::vector<std::string> titles,
                     std::vector<GURL> urls,
                     const std::vector<mojom::HistoryEntryPtr>& results) {
    SCOPED_TRACE(location.ToString());

    ASSERT_EQ(results.size(), titles.size());
    for (size_t i = 0; i < results.size(); i++) {
      EXPECT_EQ(results[i]->title, titles[i]);
      EXPECT_EQ(results[i]->url, urls[i]);
    }
  }

 protected:
  int add_count_ = 0;

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  base::FilePath history_dir_;
  std::unique_ptr<history::HistoryService> history_service_;
  std::unique_ptr<HistoryUIHandler> history_ui_handler_;
};

TEST_F(HistoryUIHandlerTest, EmptyHistory) {
  // Should return empty list when no history exists
  auto history = GetHistory();
  EXPECT_TRUE(history.empty());
}

TEST_F(HistoryUIHandlerTest, GetMultipleHistoryEntries) {
  // Add multiple history entries
  AddTestHistoryEntry("Example 1", GURL("https://example1.com"));
  AddTestHistoryEntry("Example 2", GURL("https://example2.com"));
  AddTestHistoryEntry("Example 3", GURL("https://example3.com"));

  auto history = GetHistory();
  ASSERT_EQ(3u, history.size());

  ExpectResults(FROM_HERE, {"Example 1", "Example 2", "Example 3"},
                {GURL("https://example1.com"), GURL("https://example2.com"),
                 GURL("https://example3.com")},
                history);
}

TEST_F(HistoryUIHandlerTest, SearchWithQuery) {
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

  ExpectResults(FROM_HERE, {"Brave Browser", "Brave Search"},
                {GURL("https://brave.com"), GURL("https://search.brave.com")},
                history);
}

TEST_F(HistoryUIHandlerTest, MaxResultsLimit) {
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

TEST_F(HistoryUIHandlerTest, DefaultMaxResults) {
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

TEST_F(HistoryUIHandlerTest, EmptyQuery) {
  // Add history entries
  AddTestHistoryEntry("Entry 1", GURL("https://example1.com"));
  AddTestHistoryEntry("Entry 2", GURL("https://example2.com"));

  // Empty query should return all results
  auto history = GetHistory("");

  EXPECT_EQ(2u, history.size());
}

TEST_F(HistoryUIHandlerTest, URLMatch) {
  // Add history entries
  AddTestHistoryEntry("Entry 1", GURL("https://example1.com"));
  AddTestHistoryEntry("Entry 2", GURL("https://example2.com"));

  // URL match should return just entry 2
  auto history = GetHistory("example2");

  ExpectResults(FROM_HERE, {"Entry 2"}, {GURL("https://example2.com")},
                history);
}

}  // namespace ai_chat
