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
#include "third_party/sqlite/sqlite3.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PublisherInfoDatabaseTest.*

namespace brave_rewards {

class PublisherInfoDatabaseTest : public ::testing::Test {
 protected:
  PublisherInfoDatabaseTest() {
  }

  ~PublisherInfoDatabaseTest() override {
  }

  sql::Database& GetDB() {
    return publisher_info_database_->GetDB();
  }

  void CreateTempDatabase(base::ScopedTempDir* temp_dir,
                          base::FilePath* db_file) {
    ASSERT_TRUE(temp_dir->CreateUniqueTempDir());
    *db_file = temp_dir->GetPath().AppendASCII("PublisherInfoDatabaseTest.db");
    sql::Database::Delete(*db_file);

    publisher_info_database_ =
        std::make_unique<PublisherInfoDatabase>(*db_file);
    ASSERT_NE(publisher_info_database_, nullptr);
  }

  int CountTableRows(const std::string& table) {
    std::string sql = "SELECT COUNT(*) FROM " + table;
    sql::Statement s(GetDB().GetUniqueStatement(sql.c_str()));

    if (!s.Step()) {
      return -1;
    }

    return static_cast<int>(s.ColumnInt64(0));
  }

  std::unique_ptr<PublisherInfoDatabase> publisher_info_database_;
};

TEST_F(PublisherInfoDatabaseTest, InsertContributionInfo) {
  /**
   * Good path
   */
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

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
  EXPECT_EQ(CountTableRows("contribution_info"), 1);
  EXPECT_EQ(info_sql.ColumnString(0), info.publisher_key);
  EXPECT_EQ(info_sql.ColumnString(1), info.probi);
  EXPECT_EQ(info_sql.ColumnInt64(2), info.date);
  EXPECT_EQ(info_sql.ColumnInt(3), info.category);
  EXPECT_EQ(info_sql.ColumnInt(4), info.month);
  EXPECT_EQ(info_sql.ColumnInt(5), info.year);
}

TEST_F(PublisherInfoDatabaseTest, InsertOrUpdatePublisherInfo) {
  /**
   * Good path
   */
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  ledger::PublisherInfo info;
  info.id = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  info.verified = false;
  info.excluded = ledger::PUBLISHER_EXCLUDE::DEFAULT;
  info.name = "name";
  info.url = "https://brave.com";
  info.provider = "";
  info.favicon_url = "";

  bool success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_TRUE(success);

  std::string query = "SELECT * FROM publisher_info WHERE publisher_id=?";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));

  info_sql.BindString(0, info.id);

  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(CountTableRows("publisher_info"), 1);
  EXPECT_EQ(info_sql.ColumnString(0), info.id);
  EXPECT_EQ(info_sql.ColumnBool(1), info.verified);
  EXPECT_EQ(static_cast<ledger::PUBLISHER_EXCLUDE>(info_sql.ColumnInt(2)),
      info.excluded);
  EXPECT_EQ(info_sql.ColumnString(3), info.name);
  EXPECT_EQ(info_sql.ColumnString(4), info.favicon_url);
  EXPECT_EQ(info_sql.ColumnString(5), info.url);
  EXPECT_EQ(info_sql.ColumnString(6), info.provider);

  /**
   * Make sure that second insert is update and not insert
   */
  info.verified = true;
  info.excluded = ledger::PUBLISHER_EXCLUDE::ALL;
  info.name = "updated";
  info.url = "https://clifton.com";
  info.provider = "";
  info.favicon_url = "1";

  success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_TRUE(success);

  query = "SELECT * FROM publisher_info WHERE publisher_id=?";
  sql::Statement info_sql_1(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_1.BindString(0, info.id);

  EXPECT_TRUE(info_sql_1.Step());
  EXPECT_EQ(CountTableRows("publisher_info"), 1);
  EXPECT_EQ(info_sql_1.ColumnString(0), info.id);
  EXPECT_EQ(info_sql_1.ColumnBool(1), info.verified);
  EXPECT_EQ(static_cast<ledger::PUBLISHER_EXCLUDE>(info_sql_1.ColumnInt(2)),
      info.excluded);
  EXPECT_EQ(info_sql_1.ColumnString(3), info.name);
  EXPECT_EQ(info_sql_1.ColumnString(4), info.favicon_url);
  EXPECT_EQ(info_sql_1.ColumnString(5), info.url);
  EXPECT_EQ(info_sql_1.ColumnString(6), info.provider);

  /**
   * Publisher key is missing
   */
  info.id = "";

  success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_FALSE(success);

  query = "SELECT * FROM publisher_info WHERE publisher_id=?";
  sql::Statement info_sql_2(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_2.BindString(0, info.id);

  EXPECT_FALSE(info_sql_2.Step());
}

TEST_F(PublisherInfoDatabaseTest, InsertOrUpdateActivityInfo) {
  /**
   * Good path
   */
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  ledger::PublisherInfo info;
  info.id = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  info.verified = false;
  info.excluded = ledger::PUBLISHER_EXCLUDE::DEFAULT;
  info.name = "name";
  info.url = "https://brave.com";
  info.provider = "";
  info.favicon_url = "";
  info.duration = 10;
  info.score = 1.1;
  info.percent = 100;
  info.weight = 1.5;
  info.month = ledger::ACTIVITY_MONTH::JANUARY;
  info.year = 1970;
  info.reconcile_stamp = 0;
  info.visits = 1;

  bool success = publisher_info_database_->InsertOrUpdateActivityInfo(info);
  EXPECT_TRUE(success);

  std::string query = "SELECT * FROM activity_info WHERE publisher_id=?";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));

  info_sql.BindString(0, info.id);

  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(CountTableRows("activity_info"), 1);
  EXPECT_EQ(info_sql.ColumnString(0), info.id);
  EXPECT_EQ(static_cast<uint64_t>(info_sql.ColumnInt64(1)), info.duration);
  EXPECT_EQ(info_sql.ColumnInt64(2), info.visits);
  EXPECT_EQ(info_sql.ColumnDouble(3), info.score);
  EXPECT_EQ(info_sql.ColumnInt64(4), info.percent);
  EXPECT_EQ(info_sql.ColumnDouble(5), info.weight);
  EXPECT_EQ(info_sql.ColumnInt(6), info.month);
  EXPECT_EQ(info_sql.ColumnInt(7), info.year);
  EXPECT_EQ(static_cast<uint64_t>(info_sql.ColumnInt64(8)),
            info.reconcile_stamp);

  /**
   * Make sure that second insert is update and not insert,
   * month, year and stamp is unique key
   */
  info.verified = true;
  info.excluded = ledger::PUBLISHER_EXCLUDE::ALL;
  info.name = "update";
  info.url = "https://slo-tech.com";
  info.provider = "1";
  info.favicon_url = "1";
  info.duration = 11;
  info.score = 2.1;
  info.percent = 200;
  info.weight = 2.5;
  info.visits = 2;

  success = publisher_info_database_->InsertOrUpdateActivityInfo(info);
  EXPECT_TRUE(success);

  query = "SELECT * FROM activity_info WHERE publisher_id=?";
  sql::Statement info_sql_1(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_1.BindString(0, info.id);

  EXPECT_TRUE(info_sql_1.Step());
  EXPECT_EQ(CountTableRows("activity_info"), 1);
  EXPECT_EQ(info_sql_1.ColumnString(0), info.id);
  EXPECT_EQ(static_cast<uint64_t>(info_sql_1.ColumnInt64(1)), info.duration);
  EXPECT_EQ(info_sql_1.ColumnInt64(2), info.visits);
  EXPECT_EQ(info_sql_1.ColumnDouble(3), info.score);
  EXPECT_EQ(info_sql_1.ColumnInt64(4), info.percent);
  EXPECT_EQ(info_sql_1.ColumnDouble(5), info.weight);
  EXPECT_EQ(info_sql_1.ColumnInt(6), info.month);
  EXPECT_EQ(info_sql_1.ColumnInt(7), info.year);
  EXPECT_EQ(static_cast<uint64_t>(info_sql_1.ColumnInt64(8)),
            info.reconcile_stamp);

}

TEST_F(PublisherInfoDatabaseTest, InsertOrUpdateMediaPublisherInfo) {
  /**
   * Good path
   */
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  std::string publisher_id = "id";
  std::string media_key = "key";

  bool success = publisher_info_database_->InsertOrUpdateMediaPublisherInfo(
      media_key,
      publisher_id);
  EXPECT_TRUE(success);

  std::string query =
      "SELECT * FROM media_publisher_info WHERE media_key=?";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));

  info_sql.BindString(0, media_key);

  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(CountTableRows("media_publisher_info"), 1);
  EXPECT_EQ(info_sql.ColumnString(0), media_key);
  EXPECT_EQ(info_sql.ColumnString(1), publisher_id);

  /**
   * Make sure that second insert is update and not insert
   */
  publisher_id = "id_new";

  success = publisher_info_database_->InsertOrUpdateMediaPublisherInfo(
      media_key,
      publisher_id);
  EXPECT_TRUE(success);

  query = "SELECT * FROM media_publisher_info WHERE media_key=?";
  sql::Statement info_sql_1(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_1.BindString(0, media_key);

  EXPECT_TRUE(info_sql_1.Step());
  EXPECT_EQ(CountTableRows("media_publisher_info"), 1);
  EXPECT_EQ(info_sql_1.ColumnString(0), media_key);
  EXPECT_EQ(info_sql_1.ColumnString(1), publisher_id);

  /**
   * Publisher key is missing
   */
  media_key = "missing";
  success = publisher_info_database_->InsertOrUpdateMediaPublisherInfo(
      media_key,
      "");
  EXPECT_FALSE(success);

  query = "SELECT * FROM media_publisher_info WHERE media_key=?";
  sql::Statement info_sql_2(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_2.BindString(0, media_key);

  EXPECT_FALSE(info_sql_2.Step());

  /**
   * Media key is missing
   */
  publisher_id = "new_stuff";
  success = publisher_info_database_->InsertOrUpdateMediaPublisherInfo(
      "",
      publisher_id);
  EXPECT_FALSE(success);

  query = "SELECT * FROM media_publisher_info WHERE publisher_id=?";
  sql::Statement info_sql_3(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_3.BindString(0, publisher_id);

  EXPECT_FALSE(info_sql_3.Step());
}

TEST_F(PublisherInfoDatabaseTest, InsertOrUpdateRecurringDonation) {
  /**
   * Good path
   */
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  brave_rewards::RecurringDonation info;
  info.publisher_key = "key";
  info.amount = 20;
  info.added_date = base::Time::Now().ToJsTime();

  bool success = publisher_info_database_->InsertOrUpdateRecurringDonation(
      info);
  EXPECT_TRUE(success);

  std::string query = "SELECT * FROM recurring_donation WHERE publisher_id=?";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));

  info_sql.BindString(0, info.publisher_key);

  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(CountTableRows("recurring_donation"), 1);
  EXPECT_EQ(info_sql.ColumnString(0), info.publisher_key);
  EXPECT_EQ(info_sql.ColumnDouble(1), info.amount);
  EXPECT_EQ(info_sql.ColumnInt64(2), info.added_date);

  /**
   * Make sure that second insert is update and not insert
   */
  info.amount = 30;

  success = publisher_info_database_->InsertOrUpdateRecurringDonation(info);
  EXPECT_TRUE(success);

  query ="SELECT * FROM recurring_donation WHERE publisher_id=?";
  sql::Statement info_sql_1(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_1.BindString(0, info.publisher_key);

  EXPECT_TRUE(info_sql_1.Step());
  EXPECT_EQ(CountTableRows("recurring_donation"), 1);
  EXPECT_EQ(info_sql_1.ColumnString(0), info.publisher_key);
  EXPECT_EQ(info_sql_1.ColumnDouble(1), info.amount);
  EXPECT_EQ(info_sql_1.ColumnInt64(2), info.added_date);

  /**
   * Publisher key is missing
   */
  info.publisher_key = "";
  success = publisher_info_database_->InsertOrUpdateRecurringDonation(info);
  EXPECT_FALSE(success);

  query = "SELECT * FROM recurring_donation WHERE publisher_id=?";
  sql::Statement info_sql_2(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_2.BindString(0, info.publisher_key);

  EXPECT_FALSE(info_sql_2.Step());
}

TEST_F(PublisherInfoDatabaseTest, GetPanelPublisher) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  /**
   * Publisher ID is missing
   */
  ledger::ActivityInfoFilter filter_1;
  EXPECT_EQ(publisher_info_database_->GetPanelPublisher(filter_1), nullptr);

  /**
   * Empty table
   */
  ledger::ActivityInfoFilter filter_2;
  filter_2.id = "test";
  EXPECT_EQ(publisher_info_database_->GetPanelPublisher(filter_2), nullptr);

  /**
   * Ignore month and year filter
   */
  ledger::PublisherInfo info_1;
  info_1.id = "brave.com";
  info_1.url = "https://brave.com";
  info_1.percent = 11;
  info_1.month = ledger::ACTIVITY_MONTH::JANUARY;
  info_1.year = 2019;
  info_1.reconcile_stamp = 10;

  bool success = publisher_info_database_->InsertOrUpdateActivityInfo(info_1);
  EXPECT_TRUE(success);

  ledger::ActivityInfoFilter filter_3;
  filter_3.id = "brave.com";
  filter_3.month = ledger::ACTIVITY_MONTH::ANY;
  filter_3.year = -1;
  filter_3.reconcile_stamp = 10;
  std::unique_ptr<ledger::PublisherInfo> result =
      publisher_info_database_->GetPanelPublisher(filter_3);
  EXPECT_TRUE(result);
  EXPECT_EQ(result->id, "brave.com");
}

TEST_F(PublisherInfoDatabaseTest, InsertOrUpdateActivityInfos) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  /**
   * Good path
   */
  ledger::PublisherInfo info_1;
  info_1.id = "brave.com";
  info_1.url = "https://brave.com";
  info_1.percent = 11;
  info_1.month = ledger::ACTIVITY_MONTH::JANUARY;
  info_1.year = 2019;
  info_1.reconcile_stamp = 10;

  ledger::PublisherInfo info_2;
  info_2.id = "clifton.io";
  info_2.url = "https://clifton.io";
  info_2.percent = 11;
  info_2.month = ledger::ACTIVITY_MONTH::JANUARY;
  info_2.year = 2019;
  info_2.reconcile_stamp = 10;

  ledger::PublisherInfoList list;
  list.push_back(info_1);
  list.push_back(info_2);

  bool success = publisher_info_database_->InsertOrUpdateActivityInfos(list);
  EXPECT_TRUE(success);

  /**
   * Empty list
   */
  ledger::PublisherInfoList list_empty;

  success = publisher_info_database_->InsertOrUpdateActivityInfos(list_empty);
  EXPECT_FALSE(success);

  /**
   * One publisher has empty ID
   */

  ledger::PublisherInfo info_3;
  info_3.id = "";
  info_3.url = "https://page.io";
  info_3.percent = 11;
  info_3.month = ledger::ACTIVITY_MONTH::JANUARY;
  info_3.year = 2019;
  info_3.reconcile_stamp = 10;

  list.push_back(info_3);

  success = publisher_info_database_->InsertOrUpdateActivityInfos(list);
  EXPECT_FALSE(success);
}

TEST_F(PublisherInfoDatabaseTest, InsertPendingContribution) {

}

}  // namespace brave_rewards
