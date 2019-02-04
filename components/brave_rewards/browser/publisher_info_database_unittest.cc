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
  info.verified = true;
  info.excluded = ledger::PUBLISHER_EXCLUDE::DEFAULT;
  info.name = "name";
  info.url = "https://brave.com";
  info.provider = "youtube";
  info.favicon_url = "favicon.ico";
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

  query = "SELECT * FROM publisher_info WHERE publisher_id=?";
  sql::Statement info_sql_0(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_0.BindString(0, info.id);

  EXPECT_TRUE(info_sql_0.Step());
  EXPECT_EQ(CountTableRows("publisher_info"), 1);
  EXPECT_EQ(info_sql_0.ColumnString(0), info.id);
  EXPECT_EQ(info_sql_0.ColumnBool(1), info.verified);
  EXPECT_EQ(static_cast<ledger::PUBLISHER_EXCLUDE>(info_sql_0.ColumnInt(2)),
      info.excluded);
  EXPECT_EQ(info_sql_0.ColumnString(3), info.name);
  EXPECT_EQ(info_sql_0.ColumnString(4), info.favicon_url);
  EXPECT_EQ(info_sql_0.ColumnString(5), info.url);
  EXPECT_EQ(info_sql_0.ColumnString(6), info.provider);

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

TEST_F(PublisherInfoDatabaseTest, InsertPendingContribution) {
  /**
   * Good path
   */
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  ledger::PendingContribution contribution1;
  contribution1.publisher_key = "key1";
  contribution1.amount = 10;
  contribution1.added_date = 10;
  contribution1.viewing_id = "fsodfsdnf23r23rn";
  contribution1.category = ledger::REWARDS_CATEGORY::AUTO_CONTRIBUTE;

  ledger::PendingContribution contribution2;
  contribution2.publisher_key = "key2";
  contribution2.amount = 20;
  contribution2.viewing_id = "aafsofdfsdnf23r23rn";
  contribution2.category = ledger::REWARDS_CATEGORY::DIRECT_DONATION;

  ledger::PendingContributionList list;
  list.list_.push_back(contribution1);
  list.list_.push_back(contribution2);

  bool success = publisher_info_database_->InsertPendingContribution(
      list);
  EXPECT_TRUE(success);

  std::string query = "SELECT * FROM pending_contribution";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));

  EXPECT_EQ(CountTableRows("pending_contribution"), 2);

  // First contribution
  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(info_sql.ColumnString(0), contribution1.publisher_key);
  EXPECT_EQ(info_sql.ColumnDouble(1), contribution1.amount);
  EXPECT_GE(info_sql.ColumnInt64(2), 20);
  EXPECT_EQ(info_sql.ColumnString(3), contribution1.viewing_id);
  EXPECT_EQ(static_cast<ledger::REWARDS_CATEGORY>(info_sql.ColumnInt(4)),
      contribution1.category);

  // Second contribution
  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(info_sql.ColumnString(0), contribution2.publisher_key);
  EXPECT_EQ(info_sql.ColumnDouble(1), contribution2.amount);
  EXPECT_GE(info_sql.ColumnInt64(2), 0);
  EXPECT_EQ(info_sql.ColumnString(3), contribution2.viewing_id);
  EXPECT_EQ(static_cast<ledger::REWARDS_CATEGORY>(info_sql.ColumnInt(4)),
      contribution2.category);
}

TEST_F(PublisherInfoDatabaseTest, GetActivityList) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  // first entry publisher
  ledger::PublisherInfo info;
  info.id = "publisher_1";
  info.name = "publisher_name_1";
  info.url = "https://publisher1.com";
  info.excluded = ledger::PUBLISHER_EXCLUDE::DEFAULT;
  info.duration = 0;
  info.verified = false;
  info.visits = 0;
  info.month = ledger::ACTIVITY_MONTH::JANUARY;
  info.year = 1970;
  info.reconcile_stamp = 1;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  // with duration
  info.id = "publisher_2";
  info.name = "publisher_name_2";
  info.url = "https://publisher2.com";
  info.excluded = ledger::PUBLISHER_EXCLUDE::DEFAULT;
  info.duration = 100;
  info.verified = false;
  info.visits = 1;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  // verified publisher
  info.id = "publisher_3";
  info.name = "publisher_name_3";
  info.url = "https://publisher3.com";
  info.excluded = ledger::PUBLISHER_EXCLUDE::DEFAULT;
  info.duration = 1;
  info.verified = true;
  info.visits = 1;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  // excluded publisher
  info.id = "publisher_4";
  info.name = "publisher_name_4";
  info.url = "https://publisher4.com";
  info.excluded = ledger::PUBLISHER_EXCLUDE::EXCLUDED;
  info.duration = 1;
  info.verified = false;
  info.visits = 1;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  // with visits
  info.id = "publisher_5";
  info.name = "publisher_name_5";
  info.url = "https://publisher5.com";
  info.excluded = ledger::PUBLISHER_EXCLUDE::DEFAULT;
  info.duration = 1;
  info.verified = false;
  info.visits = 10;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  // full
  info.id = "publisher_6";
  info.name = "publisher_name_6";
  info.url = "https://publisher6.com";
  info.excluded = ledger::PUBLISHER_EXCLUDE::INCLUDED;
  info.duration = 5000;
  info.verified = true;
  info.visits = 10;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  EXPECT_EQ(CountTableRows("activity_info"), 6);
  EXPECT_EQ(CountTableRows("publisher_info"), 6);

  /**
   * Get publisher with min_duration
  */
  ledger::PublisherInfoList list_1;
  ledger::ActivityInfoFilter filter_1;
  filter_1.min_duration = 50;
  filter_1.excluded = ledger::EXCLUDE_FILTER::FILTER_ALL;
  EXPECT_TRUE(publisher_info_database_->GetActivityList(0, 0, filter_1, &list_1));
  EXPECT_EQ(static_cast<int>(list_1.size()), 2);

  EXPECT_EQ(list_1.at(0).id, "publisher_2");
  EXPECT_EQ(list_1.at(1).id, "publisher_6");

  /**
   * Get verified publishers
  */
  ledger::PublisherInfoList list_2;
  ledger::ActivityInfoFilter filter_2;
  filter_2.non_verified = false;
  filter_2.excluded = ledger::EXCLUDE_FILTER::FILTER_ALL;
  EXPECT_TRUE(publisher_info_database_->GetActivityList(0, 0, filter_2, &list_2));
  EXPECT_EQ(static_cast<int>(list_2.size()), 2);

  EXPECT_EQ(list_2.at(0).id, "publisher_3");
  EXPECT_EQ(list_2.at(1).id, "publisher_6");

  /**
   * Get all publishers that are not excluded
  */
  ledger::PublisherInfoList list_3;
  ledger::ActivityInfoFilter filter_3;
  filter_3.excluded = ledger::EXCLUDE_FILTER::FILTER_ALL_EXCEPT_EXCLUDED;
  EXPECT_TRUE(publisher_info_database_->GetActivityList(0, 0, filter_3, &list_3));
  EXPECT_EQ(static_cast<int>(list_3.size()), 5);

  EXPECT_EQ(list_3.at(0).id, "publisher_1");
  EXPECT_EQ(list_3.at(1).id, "publisher_2");
  EXPECT_EQ(list_3.at(2).id, "publisher_3");
  EXPECT_EQ(list_3.at(3).id, "publisher_5");
  EXPECT_EQ(list_3.at(4).id, "publisher_6");

  /**
   * Get publisher with min_visits
  */
  ledger::PublisherInfoList list_4;
  ledger::ActivityInfoFilter filter_4;
  filter_4.min_visits = 5;
  filter_4.excluded = ledger::EXCLUDE_FILTER::FILTER_ALL;
  EXPECT_TRUE(publisher_info_database_->GetActivityList(0, 0, filter_4, &list_4));
  EXPECT_EQ(static_cast<int>(list_4.size()), 2);

  EXPECT_EQ(list_4.at(0).id, "publisher_5");
  EXPECT_EQ(list_4.at(1).id, "publisher_6");
}

}  // namespace brave_rewards
