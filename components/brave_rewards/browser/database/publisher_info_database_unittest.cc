/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fstream>
#include <streambuf>
#include <string>
#include <utility>

#include "brave/components/brave_rewards/browser/database/publisher_info_database.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/brave_paths.h"
#include "bat/ledger/global_constants.h"
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

  void PreparePendingContributions();

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

  void CreateMigrationDatabase(base::ScopedTempDir* temp_dir,
                               base::FilePath* db_file,
                               int start_version,
                               int end_version) {
    const std::string file_name = "publisher_info_db_v" +
        std::to_string(start_version);
    ASSERT_TRUE(temp_dir->CreateUniqueTempDir());
    *db_file = temp_dir->GetPath().AppendASCII(file_name);

    // Get test data migration file
    base::FilePath path;
    ASSERT_TRUE(base::PathService::Get(brave::DIR_TEST_DATA, &path));
    path = path.AppendASCII("rewards-data");
    ASSERT_TRUE(base::PathExists(path));
    path = path.AppendASCII("migration");
    ASSERT_TRUE(base::PathExists(path));
    path = path.AppendASCII(file_name);
    ASSERT_TRUE(base::PathExists(path));

    // Move it to temp dir
    bool result = base::CopyFile(path, *db_file);
    ASSERT_TRUE(result);
    ASSERT_TRUE(base::PathExists(*db_file));

    publisher_info_database_ =
    std::make_unique<PublisherInfoDatabase>(*db_file, end_version);
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

  std::string GetSchemaString(int version) {
    const std::string file_name =
        "publisher_info_schema_v" +
        std::to_string(version) +
        ".txt";

    // Get expected schema for this version
    base::FilePath path;
    base::PathService::Get(brave::DIR_TEST_DATA, &path);
    path = path.AppendASCII("rewards-data");
    path = path.AppendASCII("migration");
    path = path.AppendASCII(file_name);

    std::string data;
    base::ReadFileToString(path, &data);

    #if defined(OS_WIN)
      // Test data files may or may not have line endings converted to CRLF by
      // git checkout on Windows (depending on git autocrlf setting). Remove
      // CRLFs if they are there and replace with just LF, otherwise leave the
      // input data as is.
      auto split = base::SplitStringUsingSubstr(
          data,
          "\r\n",
          base::KEEP_WHITESPACE,
          base::SPLIT_WANT_NONEMPTY);

      if (split.size() > 1) {
        data = base::JoinString(split, "\n") + "\n";
      } else if (split.size() == 1) {
        bool ends_with_newline = (data.at(data.size() - 1) == '\n');
        data = split[0];
        if (ends_with_newline && data.at(data.size() - 1) != '\n') {
          data += "\n";
        }
      }
    #endif

    return data;
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
  info.month = static_cast<int>(ledger::ActivityMonth::JANUARY);
  info.year = 1970;
  info.type = static_cast<int>(ledger::RewardsType::AUTO_CONTRIBUTE);
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
  EXPECT_EQ(info_sql.ColumnInt(3), info.type);
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


  const std::string fav_icon = "1";

  ledger::PublisherInfo info;
  info.id = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  info.status = ledger::PublisherStatus::NOT_VERIFIED;
  info.excluded = ledger::PublisherExclude::DEFAULT;
  info.name = "name";
  info.url = "https://brave.com";
  info.provider = "";
  info.favicon_url = "0";

  ledger::ServerPublisherInfoList list;
  ledger::ServerPublisherInfoPtr server_info =
      ledger::ServerPublisherInfo::New();

  server_info->publisher_key = info.id;
  server_info->status = info.status;
  server_info->excluded = false;
  server_info->address = "";
  list.push_back(server_info->Clone());
  EXPECT_TRUE(
      publisher_info_database_->ClearAndInsertServerPublisherList(list));

  bool success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_TRUE(success);

  std::string query = "SELECT * FROM publisher_info WHERE publisher_id=?";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));

  info_sql.BindString(0, info.id);

  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(CountTableRows("publisher_info"), 1);
  EXPECT_EQ(info_sql.ColumnString(0), info.id);
  EXPECT_EQ(static_cast<ledger::PublisherExclude>(info_sql.ColumnInt(1)),
      info.excluded);
  EXPECT_EQ(info_sql.ColumnString(2), info.name);
  EXPECT_EQ(info_sql.ColumnString(3), info.favicon_url);
  EXPECT_EQ(info_sql.ColumnString(4), info.url);
  EXPECT_EQ(info_sql.ColumnString(5), info.provider);

  /**
   * Make sure that second insert is update and not insert
   */
  info.excluded = ledger::PublisherExclude::ALL;
  info.name = "updated";
  info.url = "https://clifton.com";
  info.provider = "";
  info.favicon_url = fav_icon;

  success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_TRUE(success);

  query = "SELECT * FROM publisher_info WHERE publisher_id=?";
  sql::Statement info_sql_1(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_1.BindString(0, info.id);

  EXPECT_TRUE(info_sql_1.Step());
  EXPECT_EQ(CountTableRows("publisher_info"), 1);
  EXPECT_EQ(info_sql_1.ColumnString(0), info.id);
  EXPECT_EQ(static_cast<ledger::PublisherExclude>(info_sql_1.ColumnInt(1)),
      info.excluded);
  EXPECT_EQ(info_sql_1.ColumnString(2), info.name);
  EXPECT_EQ(info_sql_1.ColumnString(3), info.favicon_url);
  EXPECT_EQ(info_sql_1.ColumnString(4), info.url);
  EXPECT_EQ(info_sql_1.ColumnString(5), info.provider);

  /**
   * If favicon is empty, don't update record
   */
  info.name = "updated2";
  info.favicon_url = "";

  success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_TRUE(success);

  query = "SELECT favicon, name FROM publisher_info WHERE publisher_id=?";
  sql::Statement info_sql_2(GetDB().GetUniqueStatement(query.c_str()));
  info_sql_2.BindString(0, info.id);
  EXPECT_TRUE(info_sql_2.Step());
  EXPECT_EQ(info_sql_2.ColumnString(0), fav_icon);
  EXPECT_EQ(info_sql_2.ColumnString(1), info.name);

  /**
   * If favicon is marked as clear, clear it
   */
  info.favicon_url = ledger::kClearFavicon;

  success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_TRUE(success);

  query = "SELECT favicon FROM publisher_info WHERE publisher_id=?";
  sql::Statement info_sql_3(GetDB().GetUniqueStatement(query.c_str()));
  info_sql_3.BindString(0, info.id);
  EXPECT_TRUE(info_sql_3.Step());
  EXPECT_EQ(info_sql_3.ColumnString(0), "");

  /**
   * Publisher key is missing
   */
  info.id = "";

  success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_FALSE(success);

  query = "SELECT * FROM publisher_info WHERE publisher_id=?";
  sql::Statement info_sql_4(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_4.BindString(0, info.id);

  EXPECT_FALSE(info_sql_4.Step());
}

TEST_F(PublisherInfoDatabaseTest, GetExcludedList) {
  /**
   * Good path
   */
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  std::string excluded_id = "ABCD";
  auto excluded_verified = ledger::PublisherStatus::VERIFIED;
  ledger::PublisherExclude excluded  = ledger::PublisherExclude::EXCLUDED;
  std::string excluded_name  = "excluded_publisher";
  std::string excluded_url  = "https://iamexcluded.com";
  std::string excluded_provider  = "exclusion";
  std::string excluded_favicon  = "0";

  std::string included_id = "EFGH";
  auto included_verified =  ledger::PublisherStatus::NOT_VERIFIED;
  ledger::PublisherExclude included  = ledger::PublisherExclude::INCLUDED;
  std::string included_name  = "included_publisher";
  std::string included_url  = "https://iamincluded.com";
  std::string included_provider  = "inclusion";
  std::string included_favicon  = "1";

  /**
   * Insert Excluded Publisher
   */
  ledger::PublisherInfo info;
  info.id = excluded_id;
  info.status = excluded_verified;
  info.excluded = excluded;
  info.name = excluded_name;
  info.url = excluded_url;
  info.provider = excluded_provider;
  info.favicon_url = excluded_favicon;

  bool success = publisher_info_database_->InsertOrUpdateActivityInfo(info);
  EXPECT_TRUE(success);

  /**
   * Insert Included Publisher
   */
  info.id = included_id;
  info.status = included_verified;
  info.excluded = included;
  info.name = included_name;
  info.url = included_url;
  info.provider = included_provider;
  info.favicon_url = included_favicon;

  success = publisher_info_database_->InsertOrUpdateActivityInfo(info);
  EXPECT_TRUE(success);
  /**
   * Check Excluded List is correct
   */
  ledger::PublisherInfoList pub_list;
  success = publisher_info_database_->GetExcludedList(&pub_list);
  EXPECT_TRUE(success);
  EXPECT_EQ(1UL, pub_list.size());

  ASSERT_EQ(pub_list.at(0)->id, excluded_id);
  ASSERT_EQ(pub_list.at(0)->name, excluded_name);
  ASSERT_EQ(pub_list.at(0)->url, excluded_url);
  ASSERT_EQ(pub_list.at(0)->provider, excluded_provider);
  ASSERT_EQ(pub_list.at(0)->favicon_url, excluded_favicon);
  pub_list.clear();

  /**
   * Check Included List is correct
   */
  auto filter = ledger::ActivityInfoFilter::New();
  filter->excluded = ledger::ExcludeFilter::FILTER_INCLUDED;
  success = publisher_info_database_->GetActivityList(0, 0,
      std::move(filter), &pub_list);
  EXPECT_TRUE(success);
  EXPECT_EQ(1UL, pub_list.size());


  ASSERT_EQ(pub_list.at(0)->id, included_id);
  ASSERT_EQ(pub_list.at(0)->name, included_name);
  ASSERT_EQ(pub_list.at(0)->url, included_url);
  ASSERT_EQ(pub_list.at(0)->provider, included_provider);
  ASSERT_EQ(pub_list.at(0)->favicon_url, included_favicon);
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
  info.status = ledger::PublisherStatus::VERIFIED;
  info.excluded = ledger::PublisherExclude::DEFAULT;
  info.name = "name";
  info.url = "https://brave.com";
  info.provider = "youtube";
  info.favicon_url = "favicon.ico";
  info.duration = 10;
  info.score = 1.1;
  info.percent = 100;
  info.weight = 1.5;
  info.reconcile_stamp = 0;
  info.visits = 1;

  ledger::ServerPublisherInfoList list;
  ledger::ServerPublisherInfoPtr server_info =
      ledger::ServerPublisherInfo::New();

  server_info->publisher_key = info.id;
  server_info->status = info.status;
  server_info->excluded = false;
  server_info->address = "";
  list.push_back(server_info->Clone());
  EXPECT_TRUE(
      publisher_info_database_->ClearAndInsertServerPublisherList(list));

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
  EXPECT_EQ(static_cast<uint64_t>(info_sql.ColumnInt64(8)),
            info.reconcile_stamp);

  query = "SELECT * FROM publisher_info WHERE publisher_id=?";
  sql::Statement info_sql_0(GetDB().GetUniqueStatement(query.c_str()));

  info_sql_0.BindString(0, info.id);

  EXPECT_TRUE(info_sql_0.Step());
  EXPECT_EQ(CountTableRows("publisher_info"), 1);
  EXPECT_EQ(info_sql_0.ColumnString(0), info.id);
  EXPECT_EQ(static_cast<ledger::PublisherExclude>(info_sql_0.ColumnInt(1)),
      info.excluded);
  EXPECT_EQ(info_sql_0.ColumnString(2), info.name);
  EXPECT_EQ(info_sql_0.ColumnString(3), info.favicon_url);
  EXPECT_EQ(info_sql_0.ColumnString(4), info.url);
  EXPECT_EQ(info_sql_0.ColumnString(5), info.provider);

  /**
   * Make sure that second insert is update and not insert,
   * publisher_id and stamp is unique key
   */
  info.excluded = ledger::PublisherExclude::ALL;
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

TEST_F(PublisherInfoDatabaseTest, InsertOrUpdateRecurringTip) {
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

  bool success = publisher_info_database_->InsertOrUpdateRecurringTip(
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

  success = publisher_info_database_->InsertOrUpdateRecurringTip(info);
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
  success = publisher_info_database_->InsertOrUpdateRecurringTip(info);
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
  auto filter_1 = ledger::ActivityInfoFilter::New();
  EXPECT_EQ(publisher_info_database_->GetPanelPublisher(std::move(filter_1)),
            static_cast<ledger::PublisherInfoPtr>(nullptr));

  /**
   * Empty table
   */
  auto filter_2 = ledger::ActivityInfoFilter::New();
  filter_2->id = "test";
  EXPECT_EQ(publisher_info_database_->GetPanelPublisher(std::move(filter_2)),
            static_cast<ledger::PublisherInfoPtr>(nullptr));

  /**
   * Still get data if reconcile stamp is not found
   */
  auto info_1 = ledger::PublisherInfo::New();
  info_1->id = "page.com";
  info_1->url = "https://page.com";
  info_1->percent = 11;
  info_1->reconcile_stamp = 9;

  bool success = publisher_info_database_->InsertOrUpdateActivityInfo(*info_1);
  EXPECT_TRUE(success);

  auto filter_4 = ledger::ActivityInfoFilter::New();
  filter_4->id = "page.com";
  filter_4->reconcile_stamp = 10;
  ledger::PublisherInfoPtr result =
      publisher_info_database_->GetPanelPublisher(std::move(filter_4));
  EXPECT_TRUE(result);
  EXPECT_EQ(result->id, "page.com");
  EXPECT_EQ(result->percent, 0u);
}

TEST_F(PublisherInfoDatabaseTest, InsertOrUpdateActivityInfos) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  /**
   * Good path
   */
  auto info_1 = ledger::PublisherInfo::New();
  info_1->id = "brave.com";
  info_1->url = "https://brave.com";
  info_1->percent = 11;
  info_1->reconcile_stamp = 10;

  auto info_2 = ledger::PublisherInfo::New();
  info_2->id = "clifton.io";
  info_2->url = "https://clifton.io";
  info_2->percent = 11;
  info_2->reconcile_stamp = 10;

  ledger::PublisherInfoList list;
  list.push_back(std::move(info_1));
  list.push_back(std::move(info_2));

  bool success = publisher_info_database_->InsertOrUpdateActivityInfos(list);
  EXPECT_TRUE(success);

  /**
   * Empty list
   */
  ledger::PublisherInfoList list_empty;

  success = publisher_info_database_->InsertOrUpdateActivityInfos(list_empty);
  EXPECT_TRUE(success);

  /**
   * One publisher has empty ID
   */

  auto info_3 = ledger::PublisherInfo::New();
  info_3->id = "";
  info_3->url = "https://page.io";
  info_3->percent = 11;
  info_3->reconcile_stamp = 10;

  list.push_back(std::move(info_3));

  success = publisher_info_database_->InsertOrUpdateActivityInfos(list);
  EXPECT_FALSE(success);
}

TEST_F(PublisherInfoDatabaseTest, InsertPendingContribution) {
  /**
   * Good path
   */
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  auto contribution1 = ledger::PendingContribution::New();
  contribution1->publisher_key = "key1";
  contribution1->amount = 10;
  contribution1->added_date = 10;
  contribution1->viewing_id = "fsodfsdnf23r23rn";
  contribution1->type = ledger::RewardsType::AUTO_CONTRIBUTE;

  auto contribution2 = ledger::PendingContribution::New();
  contribution2->publisher_key = "key2";
  contribution2->amount = 20;
  contribution2->viewing_id = "aafsofdfsdnf23r23rn";
  contribution2->type = ledger::RewardsType::ONE_TIME_TIP;

  ledger::PendingContributionList list;
  list.push_back(contribution1->Clone());
  list.push_back(contribution2->Clone());

  bool success = publisher_info_database_->InsertPendingContribution(
      std::move(list));
  EXPECT_TRUE(success);

  std::string query = "SELECT * FROM pending_contribution";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));

  EXPECT_EQ(CountTableRows("pending_contribution"), 2);

  // First contribution
  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(info_sql.ColumnString(0), contribution1->publisher_key);
  EXPECT_EQ(info_sql.ColumnDouble(1), contribution1->amount);
  EXPECT_GE(info_sql.ColumnInt64(2), 20);
  EXPECT_EQ(info_sql.ColumnString(3), contribution1->viewing_id);
  EXPECT_EQ(static_cast<ledger::RewardsType>(info_sql.ColumnInt(4)),
      contribution1->type);

  // Second contribution
  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(info_sql.ColumnString(0), contribution2->publisher_key);
  EXPECT_EQ(info_sql.ColumnDouble(1), contribution2->amount);
  EXPECT_GE(info_sql.ColumnInt64(2), 0);
  EXPECT_EQ(info_sql.ColumnString(3), contribution2->viewing_id);
  EXPECT_EQ(static_cast<ledger::RewardsType>(info_sql.ColumnInt(4)),
      contribution2->type);
}

TEST_F(PublisherInfoDatabaseTest, GetActivityList) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  ledger::ServerPublisherInfoList list;
  ledger::ServerPublisherInfoPtr server_info =
      ledger::ServerPublisherInfo::New();

  // first entry publisher
  ledger::PublisherInfo info;
  info.id = "publisher_1";
  info.name = "publisher_name_1";
  info.url = "https://publisher1.com";
  info.excluded = ledger::PublisherExclude::DEFAULT;
  info.duration = 0;
  info.status = ledger::PublisherStatus::NOT_VERIFIED;
  info.visits = 0;
  info.reconcile_stamp = 1;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  // with duration
  info.id = "publisher_2";
  info.name = "publisher_name_2";
  info.url = "https://publisher2.com";
  info.excluded = ledger::PublisherExclude::DEFAULT;
  info.duration = 100;
  info.status = ledger::PublisherStatus::NOT_VERIFIED;
  info.visits = 1;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  // verified publisher
  info.id = "publisher_3";
  info.name = "publisher_name_3";
  info.url = "https://publisher3.com";
  info.excluded = ledger::PublisherExclude::DEFAULT;
  info.duration = 1;
  info.status = ledger::PublisherStatus::VERIFIED;
  info.visits = 1;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));
  server_info->publisher_key = info.id;
  server_info->status = info.status;
  server_info->excluded = false;
  server_info->address = "";
  list.push_back(server_info->Clone());

  // excluded publisher
  info.id = "publisher_4";
  info.name = "publisher_name_4";
  info.url = "https://publisher4.com";
  info.excluded = ledger::PublisherExclude::EXCLUDED;
  info.duration = 1;
  info.status = ledger::PublisherStatus::NOT_VERIFIED;
  info.visits = 1;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  // with visits
  info.id = "publisher_5";
  info.name = "publisher_name_5";
  info.url = "https://publisher5.com";
  info.excluded = ledger::PublisherExclude::DEFAULT;
  info.duration = 1;
  info.status = ledger::PublisherStatus::NOT_VERIFIED;
  info.visits = 10;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  // full
  info.id = "publisher_6";
  info.name = "publisher_name_6";
  info.url = "https://publisher6.com";
  info.excluded = ledger::PublisherExclude::INCLUDED;
  info.duration = 5000;
  info.status = ledger::PublisherStatus::VERIFIED;
  info.visits = 10;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));
  server_info->publisher_key = info.id;
  server_info->status = info.status;
  list.push_back(server_info->Clone());

  EXPECT_EQ(CountTableRows("activity_info"), 6);
  EXPECT_EQ(CountTableRows("publisher_info"), 6);

  EXPECT_TRUE(
      publisher_info_database_->ClearAndInsertServerPublisherList(list));

  EXPECT_EQ(CountTableRows("server_publisher_info"), 2);

  /**
   * Get publisher with min_duration
  */
  ledger::PublisherInfoList list_1;
  auto filter_1 = ledger::ActivityInfoFilter::New();
  filter_1->min_duration = 50;
  filter_1->excluded = ledger::ExcludeFilter::FILTER_ALL;
  EXPECT_TRUE(publisher_info_database_->GetActivityList(0,
                                                        0,
                                                        std::move(filter_1),
                                                        &list_1));
  EXPECT_EQ(static_cast<int>(list_1.size()), 2);

  EXPECT_EQ(list_1.at(0)->id, "publisher_2");
  EXPECT_EQ(list_1.at(1)->id, "publisher_6");

  /**
   * Get verified publishers
  */
  ledger::PublisherInfoList list_2;
  auto filter_2 = ledger::ActivityInfoFilter::New();
  filter_2->non_verified = false;
  filter_2->excluded = ledger::ExcludeFilter::FILTER_ALL;
  EXPECT_TRUE(publisher_info_database_->GetActivityList(0,
                                                        0,
                                                        std::move(filter_2),
                                                        &list_2));
  EXPECT_EQ(static_cast<int>(list_2.size()), 2);

  EXPECT_EQ(list_2.at(0)->id, "publisher_3");
  EXPECT_EQ(list_2.at(1)->id, "publisher_6");

  /**
   * Get all publishers that are not excluded
  */
  ledger::PublisherInfoList list_3;
  auto filter_3 = ledger::ActivityInfoFilter::New();
  filter_3->excluded = ledger::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED;
  EXPECT_TRUE(publisher_info_database_->GetActivityList(0,
                                                        0,
                                                        std::move(filter_3),
                                                        &list_3));
  EXPECT_EQ(static_cast<int>(list_3.size()), 5);

  EXPECT_EQ(list_3.at(0)->id, "publisher_1");
  EXPECT_EQ(list_3.at(1)->id, "publisher_2");
  EXPECT_EQ(list_3.at(2)->id, "publisher_3");
  EXPECT_EQ(list_3.at(3)->id, "publisher_5");
  EXPECT_EQ(list_3.at(4)->id, "publisher_6");

  /**
   * Get publisher with min_visits
  */
  ledger::PublisherInfoList list_4;
  auto filter_4 = ledger::ActivityInfoFilter::New();
  filter_4->min_visits = 5;
  filter_4->excluded = ledger::ExcludeFilter::FILTER_ALL;
  EXPECT_TRUE(publisher_info_database_->GetActivityList(0,
                                                        0,
                                                        std::move(filter_4),
                                                        &list_4));
  EXPECT_EQ(static_cast<int>(list_4.size()), 2);

  EXPECT_EQ(list_4.at(0)->id, "publisher_5");
  EXPECT_EQ(list_4.at(1)->id, "publisher_6");
}


TEST_F(PublisherInfoDatabaseTest, Migrationv3tov4) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateMigrationDatabase(&temp_dir, &db_file, 3, 4);
  EXPECT_TRUE(publisher_info_database_->Init());

  ledger::PublisherInfoList list;
  const std::string query = "SELECT publisher_id, visits FROM activity_info";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));
  while (info_sql.Step()) {
    auto info = ledger::PublisherInfo::New();
    info->id = info_sql.ColumnString(0);
    info->visits = info_sql.ColumnInt(1);

    list.push_back(std::move(info));
  }

  EXPECT_EQ(list.at(0)->id, "slo-tech.com");
  EXPECT_EQ(list.at(0)->visits, 5u);
  EXPECT_EQ(list.at(1)->id, "brave.com");
  EXPECT_EQ(list.at(1)->visits, 5u);
  EXPECT_EQ(publisher_info_database_->GetTableVersionNumber(), 4);

  const std::string schema = publisher_info_database_->GetSchema();
  EXPECT_EQ(schema, GetSchemaString(4));
}

TEST_F(PublisherInfoDatabaseTest, Migrationv4tov5) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateMigrationDatabase(&temp_dir, &db_file, 4, 5);
  EXPECT_TRUE(publisher_info_database_->Init());

  ledger::PublisherInfoList list;
  const std::string query = "SELECT publisher_id, visits FROM activity_info";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));
  while (info_sql.Step()) {
    auto info = ledger::PublisherInfo::New();
    info->id = info_sql.ColumnString(0);
    info->visits = info_sql.ColumnInt(1);

    list.push_back(std::move(info));
  }
  EXPECT_EQ(list.at(0)->id, "brave.com");
  EXPECT_EQ(list.at(0)->visits, 1u);
  EXPECT_EQ(list.at(1)->id, "slo-tech.com");
  EXPECT_EQ(list.at(1)->visits, 1u);
  EXPECT_EQ(list.at(2)->id, "basicattentiontoken.org");
  EXPECT_EQ(list.at(2)->visits, 3u);
  EXPECT_EQ(publisher_info_database_->GetTableVersionNumber(), 5);

  const std::string schema = publisher_info_database_->GetSchema();
  EXPECT_EQ(schema, GetSchemaString(5));
}

TEST_F(PublisherInfoDatabaseTest, Migrationv5tov6) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateMigrationDatabase(&temp_dir, &db_file, 5, 6);
  EXPECT_TRUE(publisher_info_database_->Init());

  ledger::PublisherInfoList list;
  const std::string query =
      "SELECT publisher_id, visits, "
      "duration, score, percent, weight, reconcile_stamp "
      "FROM activity_info";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));
  while (info_sql.Step()) {
    auto info = ledger::PublisherInfo::New();
    info->id = info_sql.ColumnString(0);
    info->visits = info_sql.ColumnInt(1);
    info->duration = info_sql.ColumnInt64(2);
    info->score = info_sql.ColumnDouble(3);
    info->percent = info_sql.ColumnInt64(4);
    info->weight = info_sql.ColumnDouble(5);
    info->reconcile_stamp = info_sql.ColumnInt64(6);

    list.push_back(std::move(info));
  }

  EXPECT_EQ(list.at(0)->id, "basicattentiontoken.org");
  EXPECT_EQ(list.at(0)->duration, 31u);
  EXPECT_EQ(list.at(0)->visits, 1u);
  EXPECT_NEAR(list.at(0)->score, 1.1358598545838, 0.001f);
  EXPECT_EQ(list.at(0)->percent, 26u);
  EXPECT_NEAR(list.at(0)->weight, 25.919327084376, 0.001f);
  EXPECT_EQ(list.at(0)->reconcile_stamp, 1553423066u);

  EXPECT_EQ(list.at(1)->id, "brave.com");
  EXPECT_EQ(list.at(1)->duration, 20u);
  EXPECT_EQ(list.at(1)->visits, 2u);
  EXPECT_NEAR(list.at(1)->score, 1.07471534438942, 0.001f);
  EXPECT_EQ(list.at(1)->percent, 25u);
  EXPECT_NEAR(list.at(1)->weight, 24.5240629127033, 0.001f);
  EXPECT_EQ(list.at(1)->reconcile_stamp, 1553423066u);

  EXPECT_EQ(list.at(2)->id, "slo-tech.com");
  EXPECT_EQ(list.at(2)->duration, 44u);
  EXPECT_EQ(list.at(2)->visits, 2u);
  EXPECT_NEAR(list.at(2)->score, 2.1717139356, 0.001f);
  EXPECT_EQ(list.at(2)->percent, 49u);
  EXPECT_NEAR(list.at(2)->weight, 49.556610002920678, 0.001f);
  EXPECT_EQ(list.at(2)->reconcile_stamp, 1553423066u);

  EXPECT_EQ(publisher_info_database_->GetTableVersionNumber(), 6);

  const std::string schema = publisher_info_database_->GetSchema();
  EXPECT_EQ(schema, GetSchemaString(6));
}

TEST_F(PublisherInfoDatabaseTest, Migrationv4tov6) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateMigrationDatabase(&temp_dir, &db_file, 4, 6);
  EXPECT_TRUE(publisher_info_database_->Init());

  ledger::PublisherInfoList list;
  const std::string query =
      "SELECT publisher_id, visits, duration, reconcile_stamp "
      "FROM activity_info";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));
  while (info_sql.Step()) {
    auto info = ledger::PublisherInfo::New();
    info->id = info_sql.ColumnString(0);
    info->visits = info_sql.ColumnInt(1);
    info->duration = info_sql.ColumnInt64(2);
    info->reconcile_stamp = info_sql.ColumnInt64(3);

    list.push_back(std::move(info));
  }

  EXPECT_EQ(list.at(0)->id, "basicattentiontoken.org");
  EXPECT_EQ(list.at(0)->duration, 15u);
  EXPECT_EQ(list.at(0)->visits, 3u);
  EXPECT_EQ(list.at(0)->reconcile_stamp, 1552214829u);

  EXPECT_EQ(list.at(1)->id, "brave.com");
  EXPECT_EQ(list.at(1)->duration, 10u);
  EXPECT_EQ(list.at(1)->visits, 1u);
  EXPECT_EQ(list.at(1)->reconcile_stamp, 1552214829u);

  EXPECT_EQ(list.at(2)->id, "slo-tech.com");
  EXPECT_EQ(list.at(2)->duration, 12u);
  EXPECT_EQ(list.at(2)->visits, 1u);
  EXPECT_EQ(list.at(2)->reconcile_stamp, 1552214829u);

  EXPECT_EQ(publisher_info_database_->GetTableVersionNumber(), 6);
}

TEST_F(PublisherInfoDatabaseTest, Migrationv6tov7) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateMigrationDatabase(&temp_dir, &db_file, 6, 7);

  EXPECT_TRUE(publisher_info_database_->Init());
  EXPECT_EQ(publisher_info_database_->GetTableVersionNumber(), 7);

  const std::string schema = publisher_info_database_->GetSchema();
  EXPECT_EQ(schema, GetSchemaString(7));
}

TEST_F(PublisherInfoDatabaseTest, Migrationv7tov8) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateMigrationDatabase(&temp_dir, &db_file, 7, 8);
  EXPECT_TRUE(publisher_info_database_->Init());

  EXPECT_EQ(publisher_info_database_->GetTableVersionNumber(), 8);

  const std::string schema = publisher_info_database_->GetSchema();
  EXPECT_EQ(schema, GetSchemaString(8));
}

TEST_F(PublisherInfoDatabaseTest, Migrationv7tov8_ContributionInfo) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateMigrationDatabase(&temp_dir, &db_file, 7, 8);
  EXPECT_TRUE(publisher_info_database_->Init());

  ContributionInfo contribution;
  contribution.probi = "1000000000000000000";
  contribution.month = static_cast<int>(ledger::ActivityMonth::OCTOBER);
  contribution.year = 2019;
  contribution.type = static_cast<int>(ledger::RewardsType::ONE_TIME_TIP);
  contribution.date = 1570614352;
  contribution.publisher_key = "3zsistemi.si";

  std::string query = "SELECT * FROM contribution_info WHERE publisher_id=?";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));

  info_sql.BindString(0, contribution.publisher_key);

  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(CountTableRows("contribution_info"), 1);
  EXPECT_EQ(info_sql.ColumnString(0), contribution.publisher_key);
  EXPECT_EQ(info_sql.ColumnString(1), contribution.probi);
  EXPECT_EQ(info_sql.ColumnInt64(2), contribution.date);
  EXPECT_EQ(info_sql.ColumnInt(3), contribution.type);
  EXPECT_EQ(info_sql.ColumnInt(4), contribution.month);
  EXPECT_EQ(info_sql.ColumnInt(5), contribution.year);
}

TEST_F(PublisherInfoDatabaseTest, Migrationv7tov8_PendingContribution) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateMigrationDatabase(&temp_dir, &db_file, 7, 8);
  EXPECT_TRUE(publisher_info_database_->Init());

  auto pending_contribution = ledger::PendingContribution::New();
  pending_contribution->publisher_key = "reddit.com";
  pending_contribution->amount = 1.0;
  pending_contribution->added_date = 1570614383;
  pending_contribution->viewing_id = "";
  pending_contribution->type = ledger::RewardsType::ONE_TIME_TIP;

  std::string query = "SELECT * FROM pending_contribution WHERE publisher_id=?";
  sql::Statement info_sql(GetDB().GetUniqueStatement(query.c_str()));

  info_sql.BindString(0, pending_contribution->publisher_key);

  EXPECT_TRUE(info_sql.Step());
  EXPECT_EQ(CountTableRows("pending_contribution"), 1);
  EXPECT_EQ(info_sql.ColumnString(0), pending_contribution->publisher_key);
  EXPECT_EQ(info_sql.ColumnDouble(1), pending_contribution->amount);
  EXPECT_EQ(static_cast<uint64_t>(info_sql.ColumnInt64(2)),
      pending_contribution->added_date);
  EXPECT_EQ(info_sql.ColumnString(3), pending_contribution->viewing_id);
  EXPECT_EQ(info_sql.ColumnInt(4),
      static_cast<int>(pending_contribution->type));
}

TEST_F(PublisherInfoDatabaseTest, Migrationv8tov9) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateMigrationDatabase(&temp_dir, &db_file, 8, 9);
  EXPECT_TRUE(publisher_info_database_->Init());

  EXPECT_EQ(publisher_info_database_->GetTableVersionNumber(), 9);

  const std::string schema = publisher_info_database_->GetSchema();
  EXPECT_EQ(schema, GetSchemaString(9));
}

TEST_F(PublisherInfoDatabaseTest, DeleteActivityInfo) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  ledger::PublisherInfo info;
  info.id = "publisher_1";
  info.status = ledger::PublisherStatus::VERIFIED;
  info.excluded = ledger::PublisherExclude::DEFAULT;
  info.name = "publisher1";
  info.url = "https://publisher1.com";
  info.duration = 10;
  info.score = 1.1;
  info.percent = 33;
  info.weight = 1.5;
  info.reconcile_stamp = 1;
  info.visits = 1;

  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  info.reconcile_stamp = 2;
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  info.id = "publisher_2";
  info.name = "publisher2";
  info.url = "https://publisher2.com";
  EXPECT_TRUE(publisher_info_database_->InsertOrUpdateActivityInfo(info));

  // publisher key is missing
  EXPECT_FALSE(publisher_info_database_->DeleteActivityInfo("", 2));

  // reconcile stamp is missing
  EXPECT_FALSE(publisher_info_database_->DeleteActivityInfo("publisher_1", 0));

  // publisher doesn't exist
  EXPECT_TRUE(publisher_info_database_->DeleteActivityInfo("publisher_3", 2));

  // publisher is deleted
  EXPECT_TRUE(publisher_info_database_->DeleteActivityInfo("publisher_1", 2));
  ledger::PublisherInfoList list;
  auto filter = ledger::ActivityInfoFilter::New();
  filter->excluded = ledger::ExcludeFilter::FILTER_ALL;
  EXPECT_TRUE(publisher_info_database_->GetActivityList(0, 0,
      std::move(filter), &list));
  EXPECT_EQ(static_cast<int>(list.size()), 2);

  EXPECT_EQ(list.at(0)->id, "publisher_1");
  EXPECT_EQ(list.at(0)->reconcile_stamp, 1u);
  EXPECT_EQ(list.at(1)->id, "publisher_2");
}

void PublisherInfoDatabaseTest::PreparePendingContributions() {
  // Insert publishers
  ledger::PublisherInfo info;
  info.id = "key1";
  info.status = ledger::PublisherStatus::NOT_VERIFIED;
  info.excluded = ledger::PublisherExclude::DEFAULT;
  info.name = "key1";
  info.url = "https://key1.com";
  info.provider = "";
  info.favicon_url = "";

  bool success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_TRUE(success);

  info.id = "key2";
  info.name = "key2";
  info.url = "https://key2.com";
  success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_TRUE(success);

  info.id = "key3";
  info.name = "key3";
  info.url = "https://key3.com";
  success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_TRUE(success);

  info.id = "key4";
  info.name = "key4";
  info.url = "https://key4.com";
  success = publisher_info_database_->InsertOrUpdatePublisherInfo(info);
  EXPECT_TRUE(success);

  EXPECT_EQ(CountTableRows("publisher_info"), 4);

  // Insert some pending contributions
  auto contribution1 = ledger::PendingContribution::New();
  contribution1->publisher_key = "key1";
  contribution1->amount = 10;
  contribution1->viewing_id = "fsodfsdnf23r23rn";
  contribution1->type = ledger::RewardsType::AUTO_CONTRIBUTE;

  auto contribution2 = ledger::PendingContribution::New();
  contribution2->publisher_key = "key2";
  contribution2->amount = 20;
  contribution2->viewing_id = "aafsoffdffdfsdnf23r23rn";
  contribution2->type = ledger::RewardsType::ONE_TIME_TIP;

  auto contribution3 = ledger::PendingContribution::New();
  contribution3->publisher_key = "key3";
  contribution3->amount = 30;
  contribution3->viewing_id = "aafszxfzcofdfsdnf23r23rn";
  contribution3->type = ledger::RewardsType::ONE_TIME_TIP;

  auto contribution4 = ledger::PendingContribution::New();
  contribution4->publisher_key = "key4";
  contribution4->amount = 40;
  contribution4->viewing_id = "aafsofdfs12333dnf23r23rn";
  contribution4->type = ledger::RewardsType::ONE_TIME_TIP;

  ledger::PendingContributionList list;
  list.push_back(std::move(contribution1));
  list.push_back(std::move(contribution2));
  list.push_back(std::move(contribution3));
  list.push_back(std::move(contribution4));

  success = publisher_info_database_->InsertPendingContribution(
      list);
  EXPECT_TRUE(success);
  EXPECT_EQ(CountTableRows("pending_contribution"), 4);
}

TEST_F(PublisherInfoDatabaseTest, GetPendingContributions) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  PreparePendingContributions();

  /**
   * Good path
  */
  ledger::PendingContributionInfoList select_list;
  publisher_info_database_->GetPendingContributions(&select_list);
  EXPECT_EQ(static_cast<int>(select_list.size()), 4);

  EXPECT_EQ(select_list.at(0)->publisher_key, "key1");
  EXPECT_EQ(select_list.at(1)->publisher_key, "key2");
  EXPECT_EQ(select_list.at(2)->publisher_key, "key3");
  EXPECT_EQ(select_list.at(3)->publisher_key, "key4");

  EXPECT_EQ(select_list.at(0)->url, "https://key1.com");
}

TEST_F(PublisherInfoDatabaseTest, RemovePendingContributions) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  PreparePendingContributions();

  /**
   * Good path
  */
  ledger::PendingContributionInfoList select_list;
  publisher_info_database_->GetPendingContributions(&select_list);
  EXPECT_EQ(select_list.at(0)->publisher_key, "key1");
  bool success = publisher_info_database_->RemovePendingContributions(
      "key1",
      "fsodfsdnf23r23rn",
      select_list.at(0)->added_date);
  EXPECT_TRUE(success);

  ledger::PendingContributionInfoList list;
  publisher_info_database_->GetPendingContributions(&list);
  EXPECT_EQ(static_cast<int>(list.size()), 3);

  EXPECT_EQ(list.at(0)->publisher_key, "key2");
  EXPECT_EQ(list.at(1)->publisher_key, "key3");
  EXPECT_EQ(list.at(2)->publisher_key, "key4");

  /**
   * Trying to delete not existing row
  */
  success = publisher_info_database_->RemovePendingContributions(
      "key0",
      "viewing_id",
      10);
  EXPECT_TRUE(success);
  EXPECT_EQ(CountTableRows("pending_contribution"), 3);
}


TEST_F(PublisherInfoDatabaseTest, RemoveAllPendingContributions) {
  base::ScopedTempDir temp_dir;
  base::FilePath db_file;
  CreateTempDatabase(&temp_dir, &db_file);

  PreparePendingContributions();

  /**
   * Good path
  */
  bool success = publisher_info_database_->RemoveAllPendingContributions();
  EXPECT_TRUE(success);
  EXPECT_EQ(CountTableRows("pending_contribution"), 0);
}

}  // namespace brave_rewards
