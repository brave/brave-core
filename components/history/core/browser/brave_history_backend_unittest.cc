/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/history/core/browser/history_backend.h"

#include "base/files/file_util.h"
#include "base/test/gtest_util.h"
#include "base/test/task_environment.h"
#include "components/history/core/browser/history_backend_client.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/in_memory_history_backend.h"
#include "components/history/core/test/test_history_database.h"

namespace history {

// This must be a separate object since HistoryBackend manages its lifetime.
class BraveHistoryBackendTestDelegate : public HistoryBackend::Delegate {
 public:
  BraveHistoryBackendTestDelegate() = default;
  BraveHistoryBackendTestDelegate(const BraveHistoryBackendTestDelegate&) =
      delete;
  BraveHistoryBackendTestDelegate& operator=(
      const BraveHistoryBackendTestDelegate&) = delete;

  bool CanAddURL(const GURL& url) const override { return false; }
  void NotifyProfileError(sql::InitStatus init_status,
                          const std::string& diagnostics) override {}
  void SetInMemoryBackend(
      std::unique_ptr<InMemoryHistoryBackend> backend) override {}
  void NotifyFaviconsChanged(const std::set<GURL>& page_urls,
                             const GURL& icon_url) override {}
  void NotifyURLVisited(const URLRow& url_row,
                        const VisitRow& visit_row,
                        absl::optional<int64_t> local_navigation_id) override {}
  void NotifyURLsModified(const URLRows& changed_urls) override {}
  void NotifyURLsDeleted(DeletionInfo deletion_info) override {}
  void NotifyKeywordSearchTermUpdated(const URLRow& row,
                                      KeywordID keyword_id,
                                      const std::u16string& term) override {}
  void NotifyKeywordSearchTermDeleted(URLID url_id) override {}
  void DBLoaded() override {}
};

// Inspired by |HistoryBackendTest|, minimal members left.
class BraveHistoryBackendTest : public testing::Test {
 public:
  BraveHistoryBackendTest() = default;
  BraveHistoryBackendTest(const BraveHistoryBackendTest&) = delete;
  BraveHistoryBackendTest& operator=(const BraveHistoryBackendTest&) = delete;
  ~BraveHistoryBackendTest() override = default;

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  scoped_refptr<HistoryBackend> backend_;

 private:
  friend class BraveHistoryBackendTestDelegate;

  // testing::Test
  void SetUp() override {
    if (!base::CreateNewTempDirectory(FILE_PATH_LITERAL("BackendTest"),
                                      &test_dir_)) {
      return;
    }
    backend_ = base::MakeRefCounted<HistoryBackend>(
        std::make_unique<BraveHistoryBackendTestDelegate>(),
        /*backend_client*/ nullptr,
        base::SingleThreadTaskRunner::GetCurrentDefault());
    backend_->Init(false, TestHistoryDatabaseParamsForPath(test_dir_));
  }

  void TearDown() override {
    if (backend_) {
      backend_->Closing();
    }
    backend_ = nullptr;

    base::DeletePathRecursively(test_dir_);
    base::RunLoop().RunUntilIdle();
  }

  base::FilePath test_dir_;
};

TEST_F(BraveHistoryBackendTest, ExpireDaysThreshold60) {
  EXPECT_EQ(HistoryBackend::kExpireDaysThreshold, 60);

  EXPECT_EQ(backend_->expire_backend()->expiration_threshold_,
            base::Days(HistoryBackend::kExpireDaysThreshold));
}
}  // namespace history
