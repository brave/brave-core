/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ephemeral_storage/browsing_history_cleaner.h"

#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/sync/test/test_sync_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ephemeral_storage {

class BrowsingHistoryCleanerTest : public testing::Test {
 protected:
  void SetUp() override {
    TestingProfile::Builder profile_builder;
    profile_builder.AddTestingFactory(
        HistoryServiceFactory::GetInstance(),
        HistoryServiceFactory::GetDefaultFactory());
    profile_builder.AddTestingFactory(
        SyncServiceFactory::GetInstance(),
        base::BindRepeating(
            [](content::BrowserContext*) -> std::unique_ptr<KeyedService> {
              return std::make_unique<syncer::TestSyncService>();
            }));
    profile_ = profile_builder.Build();
    history_service_ = HistoryServiceFactory::GetForProfile(
        profile_.get(), ServiceAccessType::EXPLICIT_ACCESS);
    sync_service_ = static_cast<syncer::TestSyncService*>(
        SyncServiceFactory::GetForProfile(profile_.get()));
    browsing_history_cleaner_ =
        std::make_unique<ephemeral_storage::BrowsingHistoryCleaner>(
            profile_.get(), history_service_, sync_service_);
  }

  // Returns true, if the given URL exists in the history service.
  bool HistoryContainsURL(const GURL& url) {
    base::CancelableTaskTracker tracker;
    auto future = base::test::TestFuture<history::QueryURLResult>();
    history_service_->QueryURL(url, future.GetCallback(), &tracker);
    history::QueryURLResult result = future.Get<history::QueryURLResult>();
    return result.success;
  }

  void AddHistory(const GURL& url) {
    history_service_->AddPage(url, base::Time::Now() - base::Days(1), 0, 0,
                              GURL(), history::RedirectList(),
                              ui::PAGE_TRANSITION_LINK, history::SOURCE_BROWSED,
                              history::VisitResponseCodeCategory::kNot404,
                              false);
  }

  void SetOnQueryCompleteCallbackForTesting(base::OnceClosure callback) {
    browsing_history_cleaner_->SetOnQueryCompleteCallbackForTesting(
        std::move(callback));
  }

  Profile* profile() { return profile_.get(); }
  history::HistoryService* history_service() { return history_service_; }
  syncer::TestSyncService* sync_service() { return sync_service_; }
  ephemeral_storage::BrowsingHistoryCleaner* browsing_history_cleaner() {
    return browsing_history_cleaner_.get();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  raw_ptr<history::HistoryService> history_service_ = nullptr;
  raw_ptr<syncer::TestSyncService> sync_service_ = nullptr;
  std::unique_ptr<ephemeral_storage::BrowsingHistoryCleaner>
      browsing_history_cleaner_;
};
TEST_F(BrowsingHistoryCleanerTest, SimpleCleanup) {
  GURL url1("https://a.com");
  AddHistory(url1);
  EXPECT_TRUE(HistoryContainsURL(url1));
  auto on_cleanup_finished_future = base::test::TestFuture<void>();
  SetOnQueryCompleteCallbackForTesting(
      on_cleanup_finished_future.GetCallback());
  browsing_history_cleaner()->CleanupBrowsingHistoryForDomain("a.com");
  ASSERT_TRUE(on_cleanup_finished_future.Wait());

  EXPECT_FALSE(HistoryContainsURL(url1));
}

TEST_F(BrowsingHistoryCleanerTest, RemoveOnlyFullyMatchedDomains) {
  GURL url1("https://a.com");
  GURL url2("https://nota.com/path");
  GURL url3("https://a.company");
  GURL url4("https://a.com/path");

  AddHistory(url1);
  AddHistory(url2);
  AddHistory(url3);
  AddHistory(url4);

  EXPECT_TRUE(HistoryContainsURL(url1));
  EXPECT_TRUE(HistoryContainsURL(url2));
  EXPECT_TRUE(HistoryContainsURL(url3));
  EXPECT_TRUE(HistoryContainsURL(url4));

  auto on_cleanup_finished_future = base::test::TestFuture<void>();
  SetOnQueryCompleteCallbackForTesting(
      on_cleanup_finished_future.GetCallback());
  browsing_history_cleaner()->CleanupBrowsingHistoryForDomain("a.com");
  ASSERT_TRUE(on_cleanup_finished_future.Wait());

  EXPECT_FALSE(HistoryContainsURL(url1));
  EXPECT_TRUE(HistoryContainsURL(url2));
  EXPECT_TRUE(HistoryContainsURL(url3));
  EXPECT_FALSE(HistoryContainsURL(url4));
}

TEST_F(BrowsingHistoryCleanerTest, MultipleRemoveRequests) {
  GURL url1("https://a.com");
  GURL url2("https://b.com");
  GURL url3("https://c.com");

  AddHistory(url1);
  AddHistory(url2);
  AddHistory(url3);

  EXPECT_TRUE(HistoryContainsURL(url1));
  EXPECT_TRUE(HistoryContainsURL(url2));
  EXPECT_TRUE(HistoryContainsURL(url3));

  auto on_cleanup_finished_future = base::test::TestFuture<void>();
  SetOnQueryCompleteCallbackForTesting(
      on_cleanup_finished_future.GetCallback());
  browsing_history_cleaner()->CleanupBrowsingHistoryForDomain("A.com");
  browsing_history_cleaner()->CleanupBrowsingHistoryForDomain("b.com");
  browsing_history_cleaner()->CleanupBrowsingHistoryForDomain("");
  browsing_history_cleaner()->CleanupBrowsingHistoryForDomain("c.com");
  ASSERT_TRUE(on_cleanup_finished_future.Wait());

  EXPECT_FALSE(HistoryContainsURL(url1));
  EXPECT_FALSE(HistoryContainsURL(url2));
  EXPECT_FALSE(HistoryContainsURL(url3));
}

}  // namespace ephemeral_storage
