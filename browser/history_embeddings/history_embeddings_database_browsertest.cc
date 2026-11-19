// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/history_embeddings/core/history_embeddings_features.h"
#include "brave/components/history_embeddings/core/sql_database.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_history_embeddings {

namespace {

constexpr char kMarker[] = "brave-history-embeddings";

}  // namespace

class HistoryEmbeddingsDatabaseBrowserTest : public InProcessBrowserTest {
 public:
  HistoryEmbeddingsDatabaseBrowserTest() {
    // Keeps the service from opening the file, so only startup can touch it.
    feature_list_.InitAndDisableFeature(kHistoryEmbeddings);
  }

 protected:
  base::FilePath GetDatabasePath() {
    return browser()->GetProfile()->GetPath().Append(kHistoryEmbeddingsName);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(HistoryEmbeddingsDatabaseBrowserTest,
                       PRE_DatabaseSurvivesStartup) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  ASSERT_TRUE(base::WriteFile(GetDatabasePath(), kMarker));
}

IN_PROC_BROWSER_TEST_F(HistoryEmbeddingsDatabaseBrowserTest,
                       DatabaseSurvivesStartup) {
  // Lets the startup cleanup tasks posted in `PostProfileInit()` run.
  content::RunAllTasksUntilIdle();

  base::ScopedAllowBlockingForTesting allow_blocking;
  std::string contents;
  ASSERT_TRUE(base::ReadFileToString(GetDatabasePath(), &contents));
  EXPECT_EQ(kMarker, contents);
}

}  // namespace brave_history_embeddings
