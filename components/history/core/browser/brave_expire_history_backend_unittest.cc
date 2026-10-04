/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <set>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "components/history/core/browser/expire_history_backend.h"
#include "components/history/core/browser/history_backend_notifier.h"
#include "components/history/core/browser/history_constants.h"
#include "components/history/core/browser/history_database.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/common/pref_names.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/history/core/test/test_history_database.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace history {

namespace {

constexpr char kExpiredUrl[] = "https://expired.example/";
constexpr char kCurrentUrl[] = "https://current.example/";
constexpr char kForeverUrl[] = "https://forever.example/";

bool QueryURL(HistoryService* history_service,
              const GURL& url,
              base::CancelableTaskTracker* tracker) {
  base::test::TestFuture<QueryURLResult> future;
  history_service->QueryURL(url, future.GetCallback(), tracker);
  return future.Get().success;
}

}  // namespace

class BraveExpireHistoryBackendTest : public testing::Test,
                                      public HistoryBackendNotifier {
 public:
  BraveExpireHistoryBackendTest()
      : expirer_(this,
                 /*backend_client=*/nullptr,
                 task_environment_.GetMainThreadTaskRunner()) {}

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  ExpireHistoryBackend expirer_;

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    main_db_ = std::make_unique<TestHistoryDatabase>();
    ASSERT_EQ(sql::INIT_OK,
              main_db_->Init(temp_dir_.GetPath().Append(kHistoryFilename)));
    expirer_.SetDatabases(main_db_.get(), /*favicon_db=*/nullptr);
  }

  void TearDown() override {
    expirer_.SetDatabases(nullptr, nullptr);
    main_db_.reset();
  }

  URLID AddPageWithVisit(const GURL& url, base::Time visit_time) {
    URLRow url_row(url);
    url_row.set_last_visit(visit_time);
    url_row.set_visit_count(1);
    const URLID url_id = main_db_->AddURL(url_row);

    VisitRow visit_row;
    visit_row.url_id = url_id;
    visit_row.visit_time = visit_time;
    visit_row.transition = ui::PAGE_TRANSITION_TYPED;
    visit_row.source = SOURCE_BROWSED;
    EXPECT_TRUE(main_db_->AddVisit(&visit_row));
    return url_id;
  }

  bool HasURL(URLID url_id) {
    URLRow url_row;
    return main_db_->GetURLRow(url_id, &url_row);
  }

  void RunExpirationTimer() {
    task_environment_.FastForwardBy(base::Seconds(31));
  }

 protected:
  // HistoryBackendNotifier:
  void NotifyFaviconsChanged(const std::set<GURL>& page_urls,
                             const GURL& icon_url) override {}
  void NotifyURLVisited(VisitedURLInfo visited_url_info) override {}
  void NotifyURLsModified(const URLRows& changed_urls,
                          bool is_from_expiration) override {}
  void NotifyDeletions(DeletionInfo deletion_info) override {}
  void NotifyVisitUpdated(const VisitRow& visit,
                          VisitUpdateReason reason) override {}
  void NotifyVisitsDeleted(const std::vector<DeletedVisit>& visits) override {}

 private:
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<HistoryDatabase> main_db_;
};

TEST_F(BraveExpireHistoryBackendTest, ExpiresOldVisitsForPositiveThreshold) {
  const URLID expired_url_id =
      AddPageWithVisit(GURL(kExpiredUrl), base::Time::Now() - base::Days(2));
  const URLID current_url_id =
      AddPageWithVisit(GURL(kCurrentUrl), base::Time::Now());

  expirer_.StartExpiringOldStuff(base::Days(1));
  RunExpirationTimer();

  EXPECT_FALSE(HasURL(expired_url_id));
  EXPECT_TRUE(HasURL(current_url_id));
}

TEST_F(BraveExpireHistoryBackendTest, ForeverCancelsPendingExpiration) {
  const URLID expired_url_id =
      AddPageWithVisit(GURL(kExpiredUrl), base::Time::Now() - base::Days(2));

  expirer_.StartExpiringOldStuff(base::Days(1));
  expirer_.StartExpiringOldStuff(base::TimeDelta::Max());
  RunExpirationTimer();

  EXPECT_TRUE(HasURL(expired_url_id));
}

class BraveHistoryRetentionPrefTest : public testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    prefs_.registry()->RegisterIntegerPref(prefs::kBraveHistoryRetentionDays,
                                           90);
    history_service_ = CreateHistoryService(temp_dir_.GetPath(),
                                            /*create_db=*/true);
    ASSERT_TRUE(history_service_);
    history_service_->InitHistoryRetentionPref(&prefs_);
  }

  void TearDown() override {
    if (history_service_) {
      base::RunLoop run_loop;
      history_service_->SetOnBackendDestroyTask(run_loop.QuitClosure());
      history_service_.reset();
      run_loop.Run();
    }
  }

  void AddPageWithVisit(const GURL& url, base::Time visit_time) {
    history_service_->AddPageWithDetails(url, std::u16string(),
                                         /*visit_count=*/1,
                                         /*typed_count=*/0, visit_time,
                                         /*hidden=*/false, SOURCE_BROWSED);
    BlockUntilHistoryProcessesPendingRequests(history_service_.get());
  }

  void SetRetentionDays(int days) {
    prefs_.SetInteger(prefs::kBraveHistoryRetentionDays, days);
    BlockUntilHistoryProcessesPendingRequests(history_service_.get());
  }

  void RunExpirationTimer() {
    task_environment_.FastForwardBy(base::Seconds(31));
    BlockUntilHistoryProcessesPendingRequests(history_service_.get());
  }

  bool HasURL(const GURL& url) {
    return QueryURL(history_service_.get(), url, &tracker_);
  }

 private:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<HistoryService> history_service_;
  base::CancelableTaskTracker tracker_;
};

TEST_F(BraveHistoryRetentionPrefTest, PrefChangeExpiresOldVisits) {
  const GURL expired_url(kExpiredUrl);
  const GURL current_url(kCurrentUrl);
  AddPageWithVisit(expired_url, base::Time::Now() - base::Days(2));
  AddPageWithVisit(current_url, base::Time::Now());

  SetRetentionDays(1);
  RunExpirationTimer();

  EXPECT_FALSE(HasURL(expired_url));
  EXPECT_TRUE(HasURL(current_url));
}

TEST_F(BraveHistoryRetentionPrefTest, ForeverPrefKeepsOldVisits) {
  const GURL forever_url(kForeverUrl);
  AddPageWithVisit(forever_url, base::Time::Now() - base::Days(2));

  SetRetentionDays(-1);
  RunExpirationTimer();

  EXPECT_TRUE(HasURL(forever_url));
}

}  // namespace history
