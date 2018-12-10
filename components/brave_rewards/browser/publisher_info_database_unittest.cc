/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fstream>
#include <streambuf>

#include "brave/components/brave_rewards/browser/publisher_info_database.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "sql/transaction.h"
#include "third_party/sqlite/sqlite3.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PublisherInfoDatabaseTest.*

namespace brave_rewards {

// Test data directory, relative to source root
const base::FilePath::CharType kTestDataRelativePath[] =
  FILE_PATH_LITERAL("brave/vendor/bat-native-ledger/test/data");

class PublisherInfoDatabaseTest : public ::testing::Test {
 protected:
  PublisherInfoDatabaseTest() {
    // You can do set-up work for each test here
  }

  ~PublisherInfoDatabaseTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    base::ScopedTempDir temp_dir;
    ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
    ASSERT_TRUE(base::DirectoryExists(temp_dir.GetPath()));
    base::FilePath db_file =
      temp_dir.GetPath().AppendASCII("PublisherInfoDatabaseTest.db");

    base::FilePath temp_file;
    ASSERT_TRUE(
        base::CreateTemporaryFileInDir(temp_dir.GetPath(), &temp_file));

    publisher_info_database_ = std::make_unique<PublisherInfoDatabase>(temp_file);
    ASSERT_NE(publisher_info_database_, nullptr);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  std::string GetMockDataPath(const std::string& filename) {
    base::FilePath path(kTestDataRelativePath);
    path = path.AppendASCII(filename);
    return path.value();
  }

  sql::Database& GetDB() {
    return publisher_info_database_->GetDB();
  }

  std::unique_ptr<PublisherInfoDatabase> publisher_info_database_;
};

TEST_F(PublisherInfoDatabaseTest, InsertContributionInfo) {
  ContributionInfo info;
  info.probi = "12345678901234567890123456789012345678901234";
  info.month = ledger::ACTIVITY_MONTH::JANUARY;
  info.year = 1970;
  info.category = ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE;
  info.date = base::Time::Now().ToJsTime();
  info.publisher_key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  bool success = publisher_info_database_->InsertContributionInfo(info);
  EXPECT_TRUE(success);

  std::string query = "SELECT * FROM contribution_info WHERE publisher_id=?";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));

  info_sql.BindString(0, info.publisher_key);

  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(info_sql.ColumnString(0), info.publisher_key);
  EXPECT_EQ(info_sql.ColumnString(1), info.probi);
  EXPECT_EQ(info_sql.ColumnInt64(2), info.date);
  EXPECT_EQ(info_sql.ColumnInt(3), info.category);
  EXPECT_EQ(info_sql.ColumnInt(4), info.month);
  EXPECT_EQ(info_sql.ColumnInt(5), info.year);
}

}  // namespace brave_rewards
