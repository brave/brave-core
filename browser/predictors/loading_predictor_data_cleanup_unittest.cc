/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/predictors/loading_predictor_data_cleanup.h"

#include <array>
#include <string_view>

#include "base/strings/strcat.h"
#include "sql/database.h"
#include "sql/test/test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace predictors {

namespace {

constexpr char kHostRedirectTable[] =
    "resource_prefetch_predictor_host_redirect";
constexpr char kOriginTable[] = "resource_prefetch_predictor_origin";
constexpr char kLcppTable[] = "lcp_critical_path_predictor";
constexpr char kLcppInitiatorOriginTable[] =
    "lcp_critical_path_predictor_initiator_origin";

// The omnibox predictor's table. It shares the database file with the loading
// predictor's tables, but not their fate.
constexpr char kAutocompleteTable[] = "network_action_predictor";

constexpr auto kLoadingPredictorTables = std::to_array<const char*>({
    kHostRedirectTable,
    kOriginTable,
    kLcppTable,
    kLcppInitiatorOriginTable,
});

}  // namespace

class LoadingPredictorDataCleanupTest : public testing::Test {
 public:
  LoadingPredictorDataCleanupTest() : db_(sql::test::kTestTag) {}

 protected:
  void SetUp() override { ASSERT_TRUE(db_.OpenInMemory()); }

  void CreateTable(std::string_view table_name) {
    ASSERT_TRUE(db_.Execute(base::StrCat(
        {"CREATE TABLE ", table_name, " (key TEXT, proto BLOB)"})));
  }

  void CreateTableWithRow(std::string_view table_name) {
    CreateTable(table_name);
    ASSERT_TRUE(db_.Execute(base::StrCat({"INSERT INTO ", table_name,
                                          " (key, proto) VALUES "
                                          "('a.test', x'0102')"})));
  }

  void CreateLoadingPredictorTables(bool with_rows) {
    for (const char* table_name : kLoadingPredictorTables) {
      if (with_rows) {
        CreateTableWithRow(table_name);
      } else {
        CreateTable(table_name);
      }
    }
  }

  size_t CountRows(const char* table_name) {
    size_t count = 0;
    EXPECT_TRUE(sql::test::CountTableRows(&db_, table_name, &count));
    return count;
  }

  sql::Database db_;
};

TEST_F(LoadingPredictorDataCleanupTest, DropsCollectedData) {
  CreateLoadingPredictorTables(/*with_rows=*/true);

  MaybeClearLoadingPredictorData(&db_, /*is_loading_predictor_enabled=*/false);

  for (const char* table_name : kLoadingPredictorTables) {
    EXPECT_FALSE(db_.DoesTableExist(table_name)) << table_name;
  }
}

TEST_F(LoadingPredictorDataCleanupTest, KeepsOmniboxPredictorData) {
  CreateLoadingPredictorTables(/*with_rows=*/true);
  CreateTableWithRow(kAutocompleteTable);

  MaybeClearLoadingPredictorData(&db_, /*is_loading_predictor_enabled=*/false);

  ASSERT_TRUE(db_.DoesTableExist(kAutocompleteTable));
  EXPECT_EQ(1u, CountRows(kAutocompleteTable));
}

TEST_F(LoadingPredictorDataCleanupTest, LeavesEmptyTablesAlone) {
  CreateLoadingPredictorTables(/*with_rows=*/false);

  MaybeClearLoadingPredictorData(&db_, /*is_loading_predictor_enabled=*/false);

  // Upstream recreates the tables empty right after the drop, so a clean
  // profile must be left untouched instead of dropping them on every launch.
  for (const char* table_name : kLoadingPredictorTables) {
    EXPECT_TRUE(db_.DoesTableExist(table_name)) << table_name;
  }
}

TEST_F(LoadingPredictorDataCleanupTest, DropsEveryTableWhenOnlyOneHasData) {
  CreateLoadingPredictorTables(/*with_rows=*/false);
  ASSERT_TRUE(db_.Execute(base::StrCat({"INSERT INTO ", kLcppTable,
                                        " (key, proto) VALUES "
                                        "('a.test', x'0102')"})));

  MaybeClearLoadingPredictorData(&db_, /*is_loading_predictor_enabled=*/false);

  for (const char* table_name : kLoadingPredictorTables) {
    EXPECT_FALSE(db_.DoesTableExist(table_name)) << table_name;
  }
}

TEST_F(LoadingPredictorDataCleanupTest, HandlesMissingTables) {
  // A fresh profile: the loading predictor tables do not exist yet.
  CreateTableWithRow(kAutocompleteTable);

  MaybeClearLoadingPredictorData(&db_, /*is_loading_predictor_enabled=*/false);

  for (const char* table_name : kLoadingPredictorTables) {
    EXPECT_FALSE(db_.DoesTableExist(table_name)) << table_name;
  }
  EXPECT_EQ(1u, CountRows(kAutocompleteTable));
}

TEST_F(LoadingPredictorDataCleanupTest, KeepsDataWhenPredictorEnabled) {
  CreateLoadingPredictorTables(/*with_rows=*/true);

  MaybeClearLoadingPredictorData(&db_, /*is_loading_predictor_enabled=*/true);

  for (const char* table_name : kLoadingPredictorTables) {
    ASSERT_TRUE(db_.DoesTableExist(table_name)) << table_name;
    EXPECT_EQ(1u, CountRows(table_name)) << table_name;
  }
}

}  // namespace predictors
