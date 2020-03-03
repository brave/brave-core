/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/memory/weak_ptr.h"
#include "base/test/bind_test_util.h"
#include "bat/ledger/internal/bat_helper.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/request/request_util.h"
#include "bat/ledger/internal/static_values.h"
#include "bat/ledger/ledger.h"
#include "brave/browser/extensions/api/brave_action_api.h"
#include "brave/common/brave_paths.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"  // NOLINT
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"  // NOLINT
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/browser/locale_helper_mock.h"
#include "brave/components/brave_ads/browser/notification_helper_mock.h"
#include "brave/browser/ui/views/brave_actions/brave_actions_container.h"
#include "brave/browser/ui/views/location_bar/brave_location_bar_view.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "sql/database.h"
#include "sql/meta_table.h"
#include "sql/statement.h"

// npm run test -- brave_browser_tests --filter=RewardsDatabaseBrowserTest.*

class RewardsDatabaseBrowserTest
    : public InProcessBrowserTest,
      public brave_rewards::RewardsServiceObserver,
      public base::SupportsWeakPtr<BraveRewardsBrowserTest> {
 public:
  RewardsDatabaseBrowserTest() {
  }

  ~RewardsDatabaseBrowserTest() override {
  }

  bool SetUpUserDataDirectory() override {
    int32_t version = 0;
    GetMigrationVersionFromTest(&version);
    CopyDatabaseFile(version);
    return true;
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    brave::RegisterPathProvider();

    auto* browser_profile = browser()->profile();

    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(browser_profile));

    rewards_service_->AddObserver(this);
    if (!rewards_service_->IsWalletInitialized()) {
      WaitForWalletInitialization();
    }
    rewards_service_->SetLedgerEnvForTesting();
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void GetMigrationVersionFromTest(int32_t* version) {
    if (!version) {
      return;
    }

    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    ASSERT_NE(test_info, nullptr);

    std::string test_name = test_info->name();

    auto version_split = base::SplitStringUsingSubstr(
      test_name,
      "_",
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);

    int32_t test_version = std::stoi(version_split[1]);

    ASSERT_GT(test_version, 0);

    *version = test_version - 1;
  }

  void WaitForWalletInitialization() {
    if (wallet_initialized_) {
      return;
    }
    wait_for_wallet_initialization_loop_.reset(new base::RunLoop);
    wait_for_wallet_initialization_loop_->Run();
  }

  void OnWalletInitialized(
      brave_rewards::RewardsService* rewards_service,
      int32_t result) override {
    const auto converted_result = static_cast<ledger::Result>(result);
    ASSERT_TRUE(converted_result == ledger::Result::WALLET_CREATED ||
                converted_result == ledger::Result::NO_LEDGER_STATE ||
                converted_result == ledger::Result::LEDGER_OK);
    wallet_initialized_ = true;
    if (wait_for_wallet_initialization_loop_) {
      wait_for_wallet_initialization_loop_->Quit();
    }
  }

  base::FilePath GetUserDataPath() const {
    base::FilePath path;
    base::PathService::Get(chrome::DIR_USER_DATA, &path);
    path = path.AppendASCII(TestingProfile::kTestUserProfileDir);
    return path;
  }

  base::FilePath GetTestDataPath() const {
    base::FilePath path;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.Append(FILE_PATH_LITERAL("brave"));
    path = path.Append(FILE_PATH_LITERAL("test"));
    path = path.Append(FILE_PATH_LITERAL("data"));
    return path;
  }

  void GetDBPath(base::FilePath* path) const {
    auto user_data_path = GetUserDataPath();
    ASSERT_TRUE(base::CreateDirectory(user_data_path));
    const std::string& db_file_name = "publisher_info_db";

    user_data_path = user_data_path.AppendASCII(db_file_name);
    *path = user_data_path;
  }

  void GetTestFile(const std::string& file_name, base::FilePath* path) const {
    auto test_data_path = GetTestDataPath();
    test_data_path = test_data_path.AppendASCII("rewards-data");
    test_data_path = test_data_path.AppendASCII("migration");
    test_data_path = test_data_path.AppendASCII(file_name);
    ASSERT_TRUE(base::PathExists(test_data_path));

    *path = test_data_path;
  }

  void CopyDatabaseFile(const int32_t version) const {
    base::FilePath db_path;
    GetDBPath(&db_path);
    const std::string& db_file_name = "publisher_info_db";
    const std::string test_file_name = base::StringPrintf(
        "%s_v%d",
        db_file_name.c_str(),
        version);
    base::FilePath test_data_path;
    GetTestFile(test_file_name, &test_data_path);
    ASSERT_TRUE(base::CopyFile(test_data_path, db_path));
  }

  void InitDB(int32_t version = 0) {
    if (version == 0) {
      GetMigrationVersionFromTest(&version);
    }

    base::FilePath db_path;
    GetDBPath(&db_path);
    ASSERT_TRUE(db_.Open(db_path));
    ASSERT_TRUE(meta_table_.Init(
        &db_,
        braveledger_database::GetCurrentVersion(),
        braveledger_database::GetCompatibleVersion()));
    ASSERT_EQ(
        GetTableVersionNumber(),
        braveledger_database::GetCurrentVersion());
  }

  std::string GetSchemaString() {
    const std::string file_name = "publisher_info_schema_current.txt";

    // Get expected schema for this version
    base::FilePath path;
    GetTestFile(file_name, &path);

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

  std::string GetSchema() const {
    std::string schema = db_.GetSchema();

    // Legacy: This is needed, because old db file had publisher_info table
    // that was never recreated, so that space is still in there
    // so we need to remove it
    base::ReplaceSubstringsAfterOffset(
      &schema,
      0,
      "CREATE TABLE publisher_info (",
      "CREATE TABLE publisher_info(");

    return schema;
  }

  int32_t CountTableRows(const std::string& table) {
    const std::string sql = base::StringPrintf(
        "SELECT COUNT(*) FROM %s",
        table.c_str());
    sql::Statement s(db_.GetUniqueStatement(sql.c_str()));

    if (!s.Step()) {
      return -1;
    }

    return static_cast<int>(s.ColumnInt64(0));
  }

  int32_t GetTableVersionNumber() {
    return meta_table_.GetVersionNumber();
  }

  brave_rewards::RewardsServiceImpl* rewards_service_;

  std::unique_ptr<base::RunLoop> wait_for_wallet_initialization_loop_;
  bool wallet_initialized_ = false;
  sql::Database db_;
  sql::MetaTable meta_table_;
};

/**
 * AUTOMATED SCHEMA MIGRATION TESTS
 */
struct SchemaCheckParamInfo {
  int32_t version;
};

class SchemaCheck :
    public RewardsDatabaseBrowserTest,
    public ::testing::WithParamInterface<SchemaCheckParamInfo> {};

static std::string GetTestCaseName(
    ::testing::TestParamInfo<SchemaCheckParamInfo> param_info) {
  return base::StringPrintf("Migration_%d", param_info.param.version);
}

std::vector<SchemaCheckParamInfo> GetTestData() {
  std::vector<SchemaCheckParamInfo> data;

  const int32_t min_migration_version = 4;
  const auto version = braveledger_database::GetCurrentVersion();
  for (int32_t i = min_migration_version; i <= version; i++) {
    data.push_back({
      i
    });
  }

  return data;
}

IN_PROC_BROWSER_TEST_P(SchemaCheck, PerVersion) {
  SchemaCheckParamInfo param(GetParam());
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    InitDB(param.version);

    const std::string schema = GetSchema();
    EXPECT_EQ(schema, GetSchemaString());
  }
}

INSTANTIATE_TEST_CASE_P(
    RewardsDatabaseBrowserTest,
    SchemaCheck,
    ::testing::ValuesIn(GetTestData()), GetTestCaseName);

/**
 * TABLE SPECIFIC MIGRATION TESTS
 */
IN_PROC_BROWSER_TEST_F(RewardsDatabaseBrowserTest, Migration_4_ActivityInfo) {
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    InitDB();

    ledger::PublisherInfoList list;
    const std::string query = "SELECT publisher_id, visits FROM activity_info";
    sql::Statement info_sql(db_.GetUniqueStatement(query.c_str()));
    while (info_sql.Step()) {
      auto info = ledger::PublisherInfo::New();
      info->id = info_sql.ColumnString(0);
      info->visits = info_sql.ColumnInt(1);

      list.push_back(std::move(info));
    }

    EXPECT_EQ(list.at(0)->id, "brave.com");
    EXPECT_EQ(list.at(0)->visits, 5u);
    EXPECT_EQ(list.at(1)->id, "slo-tech.com");
    EXPECT_EQ(list.at(1)->visits, 5u);
  }
}

IN_PROC_BROWSER_TEST_F(RewardsDatabaseBrowserTest, Migration_5_ActivityInfo) {
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    InitDB();

    ledger::PublisherInfoList list;
    const std::string query = "SELECT publisher_id, visits FROM activity_info";
    sql::Statement info_sql(db_.GetUniqueStatement(query.c_str()));
    while (info_sql.Step()) {
      auto info = ledger::PublisherInfo::New();
      info->id = info_sql.ColumnString(0);
      info->visits = info_sql.ColumnInt(1);

      list.push_back(std::move(info));
    }
    EXPECT_EQ(list.at(0)->id, "basicattentiontoken.org");
    EXPECT_EQ(list.at(0)->visits, 3u);
    EXPECT_EQ(list.at(1)->id, "brave.com");
    EXPECT_EQ(list.at(1)->visits, 1u);
    EXPECT_EQ(list.at(2)->id, "slo-tech.com");
    EXPECT_EQ(list.at(2)->visits, 1u);
  }
}

IN_PROC_BROWSER_TEST_F(RewardsDatabaseBrowserTest, Migration_6_ActivityInfo) {
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    InitDB();

    ledger::PublisherInfoList list;
    const std::string query =
        "SELECT publisher_id, visits, "
        "duration, score, percent, weight, reconcile_stamp "
        "FROM activity_info";
    sql::Statement info_sql(db_.GetUniqueStatement(query.c_str()));
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
  }
}

IN_PROC_BROWSER_TEST_F(
    RewardsDatabaseBrowserTest,
    Migration_8_PendingContribution) {
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    InitDB();

    auto pending_contribution = ledger::PendingContribution::New();
    pending_contribution->publisher_key = "reddit.com";
    pending_contribution->amount = 1.0;
    pending_contribution->added_date = 1570614383;
    pending_contribution->viewing_id = "";
    pending_contribution->type = ledger::RewardsType::ONE_TIME_TIP;

    std::string query =
        "SELECT publisher_id, amount, added_date, viewing_id, type "
        "FROM pending_contribution WHERE publisher_id=?";
    sql::Statement info_sql(db_.GetUniqueStatement(query.c_str()));

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
}

IN_PROC_BROWSER_TEST_F(
    RewardsDatabaseBrowserTest,
    Migration_11_ContributionInfo) {
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    InitDB();

    EXPECT_EQ(CountTableRows("contribution_info"), 5);
    EXPECT_EQ(CountTableRows("contribution_info_publishers"), 4);

    const std::string query =
        "SELECT ci.contribution_id, ci.amount, ci.type, ci.created_at, "
        "cip.publisher_key, cip.total_amount, cip.contributed_amount "
        "FROM contribution_info as ci "
        "LEFT JOIN contribution_info_publishers AS cip "
        "ON ci.contribution_id = cip.contribution_id "
        "WHERE ci.contribution_id LIKE ?";

    // One time tip
    const std::string tip_id = "id_1570614352_%";
    sql::Statement tip_sql(db_.GetUniqueStatement(query.c_str()));
    tip_sql.BindString(0, tip_id);

    ASSERT_TRUE(tip_sql.Step());
    EXPECT_EQ(tip_sql.ColumnDouble(1), 1.0);
    EXPECT_EQ(tip_sql.ColumnInt(2),
        static_cast<int>(ledger::RewardsType::ONE_TIME_TIP));;
    EXPECT_EQ(tip_sql.ColumnInt64(3), 1570614352);
    EXPECT_EQ(tip_sql.ColumnString(4), "3zsistemi.si");
    EXPECT_EQ(tip_sql.ColumnDouble(5), 1.0);
    EXPECT_EQ(tip_sql.ColumnDouble(6), 1.0);

    // Auto contribute
    const std::string ac_id = "id_1574671381_%";
    sql::Statement ac_sql(db_.GetUniqueStatement(query.c_str()));
    ac_sql.BindString(0, ac_id);

    ASSERT_TRUE(ac_sql.Step());
    EXPECT_EQ(ac_sql.ColumnDouble(1), 10.0);
    EXPECT_EQ(ac_sql.ColumnInt(2),
        static_cast<int>(ledger::RewardsType::AUTO_CONTRIBUTE));;
    EXPECT_EQ(ac_sql.ColumnInt64(3), 1574671381);
    EXPECT_EQ(ac_sql.ColumnString(4), "");
    EXPECT_EQ(ac_sql.ColumnDouble(5), 0.0);
    EXPECT_EQ(ac_sql.ColumnDouble(6), 0.0);
  }
}

IN_PROC_BROWSER_TEST_F(
    RewardsDatabaseBrowserTest,
    Migration_12_ContributionInfo) {
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    InitDB();

    EXPECT_EQ(CountTableRows("pending_contribution"), 4);

    ledger::PendingContributionInfoList list;
    const std::string query =
        "SELECT pending_contribution_id, publisher_id "
        "FROM pending_contribution";
    sql::Statement info_sql(db_.GetUniqueStatement(query.c_str()));
    while (info_sql.Step()) {
      auto info = ledger::PendingContributionInfo::New();
      info->id = info_sql.ColumnInt64(0);
      info->publisher_key = info_sql.ColumnString(1);

      list.push_back(std::move(info));
    }
    EXPECT_EQ(static_cast<int>(list.size()), 4);

    EXPECT_EQ(list.at(0)->id, 1ull);
    EXPECT_EQ(list.at(0)->publisher_key, "reddit.com");
    EXPECT_EQ(list.at(1)->id, 4ull);
    EXPECT_EQ(list.at(1)->publisher_key, "reddit.com");
    EXPECT_EQ(list.at(2)->id, 2ull);
    EXPECT_EQ(list.at(2)->publisher_key, "slo-tech.com");
    EXPECT_EQ(list.at(3)->id, 3ull);
    EXPECT_EQ(list.at(3)->publisher_key, "slo-tech.com");
  }
}

IN_PROC_BROWSER_TEST_F(RewardsDatabaseBrowserTest, Migration_13_Promotion) {
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    InitDB();

    EXPECT_EQ(CountTableRows("promotion"), 1);
  }
}

IN_PROC_BROWSER_TEST_F(
    RewardsDatabaseBrowserTest,
    Migration_14_UnblindedToken) {
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    InitDB();

    EXPECT_EQ(CountTableRows("unblinded_tokens"), 5);

    ledger::UnblindedTokenList list;
    std::string query = "SELECT value FROM unblinded_tokens";
    sql::Statement tokens_sql(db_.GetUniqueStatement(query.c_str()));
    while (tokens_sql.Step()) {
      auto info = ledger::UnblindedToken::New();
      info->value = tokens_sql.ColumnDouble(0);

      list.push_back(std::move(info));
    }

    EXPECT_EQ(list.at(0)->value, 0.25);
    EXPECT_EQ(list.at(1)->value, 0.25);
    EXPECT_EQ(list.at(2)->value, 0.25);
    EXPECT_EQ(list.at(3)->value, 0.25);
    EXPECT_EQ(list.at(4)->value, 0.25);

    query = "SELECT approximate_value FROM promotion WHERE promotion_id=?";
    const std::string promotion_id = "36baa4c3-f92d-4121-b6d9-db44cb273a02";

    sql::Statement promotion_sql(db_.GetUniqueStatement(query.c_str()));
    promotion_sql.BindString(0, promotion_id);
    ASSERT_TRUE(promotion_sql.Step());

    EXPECT_EQ(promotion_sql.ColumnDouble(0), 1.25);
  }
}
