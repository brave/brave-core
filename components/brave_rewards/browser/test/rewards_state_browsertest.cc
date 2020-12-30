/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind_test_util.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_test.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsStateBrowserTest.*

namespace rewards_browsertest {

class RewardsStateBrowserTest : public InProcessBrowserTest {
 public:
  RewardsStateBrowserTest() {
    response_ = std::make_unique<RewardsBrowserTestResponse>();
  }

  bool SetUpUserDataDirectory() override {
    int32_t current_version = 0;
    GetMigrationVersionFromTest(&current_version);
    CopyPublisherFile(current_version);
    CopyStateFile(current_version);
    return true;
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // HTTP resolver
    https_server_.reset(new net::EmbeddedTestServer(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS));
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&rewards_browsertest_util::HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    // Rewards service
    brave::RegisterPathProvider();
    profile_ = browser()->profile();
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile_));

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &RewardsStateBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetLedgerEnvForTesting();

    // Bypass onboarding UX by default
    rewards_service_->SaveOnboardingResult(
        brave_rewards::OnboardingResult::kDismissed);
  }

  void GetTestResponse(
      const std::string& url,
      int32_t method,
      int* response_status_code,
      std::string* response,
      base::flat_map<std::string, std::string>* headers) {
    response_->Get(
        url,
        method,
        response_status_code,
        response);
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

    if (version_split.size() != 2) {
      return;
    }

    int32_t test_version = std::stoi(version_split[1]);

    ASSERT_GT(test_version, 0);

    *version = test_version - 1;
  }

  base::FilePath GetUserDataPath() const {
    base::FilePath path;
    base::PathService::Get(chrome::DIR_USER_DATA, &path);
    path = path.AppendASCII(TestingProfile::kTestUserProfileDir);
    return path;
  }

  void GetFilePath(const std::string& file_name, base::FilePath* path) const {
    auto user_data_path = GetUserDataPath();
    ASSERT_TRUE(base::CreateDirectory(user_data_path));

    user_data_path = user_data_path.AppendASCII(file_name);
    *path = user_data_path;
  }

  void GetTestFile(const std::string& file_name, base::FilePath* path) const {
    base::FilePath test_path;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &test_path);
    test_path = test_path.Append(FILE_PATH_LITERAL("brave"));
    test_path = test_path.Append(FILE_PATH_LITERAL("test"));
    test_path = test_path.Append(FILE_PATH_LITERAL("data"));
    test_path = test_path.Append(FILE_PATH_LITERAL("rewards-data"));
    test_path = test_path.Append(FILE_PATH_LITERAL("state"));
    test_path = test_path.AppendASCII(file_name);
    ASSERT_TRUE(base::PathExists(test_path));

    *path = test_path;
  }

  void CopyPublisherFile(const int32_t current_version) const {
    if (current_version != 0) {
      return;
    }

    base::FilePath profile_path;
    GetFilePath("publisher_state", &profile_path);
    base::FilePath test_path;
    GetTestFile("publisher_state", &test_path);
    ASSERT_TRUE(base::CopyFile(test_path, profile_path));
  }

  void CopyStateFile(const int32_t current_version) const {
    if (current_version != 1) {
      return;
    }

    base::FilePath profile_path;
    GetFilePath("ledger_state", &profile_path);
    base::FilePath test_path;
    GetTestFile("ledger_state", &test_path);
    ASSERT_TRUE(base::CopyFile(test_path, profile_path));
  }

  brave_rewards::RewardsServiceImpl* rewards_service_;
  Profile* profile_;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<RewardsBrowserTestResponse> response_;
};

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, State_1) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", -1);
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetInteger("brave.rewards.ac.min_visit_time"),
      5);
  EXPECT_EQ(
      profile_->GetPrefs()->GetInteger("brave.rewards.ac.min_visits"),
      5);
  EXPECT_EQ(profile_->GetPrefs()->GetBoolean(
      "brave.rewards.ac.allow_non_verified"),
          false);
  EXPECT_EQ(profile_->GetPrefs()->GetBoolean(
      "brave.rewards.ac.allow_video_contributions"),
          false);
  EXPECT_EQ(profile_->GetPrefs()->GetDouble("brave.rewards.ac.score.a"),
      14500.0);
  EXPECT_EQ(profile_->GetPrefs()->GetDouble("brave.rewards.ac.score.b"),
      -14000.0);

  rewards_service_->GetBalanceReport(
      4,
      2020,
      base::BindLambdaForTesting(
          [&](
              const ledger::type::Result result,
              ledger::type::BalanceReportInfoPtr report) {
        EXPECT_EQ(report->grants, 4.1);
        EXPECT_EQ(report->earning_from_ads, 4.2);
        EXPECT_EQ(report->auto_contribute, 4.3);
        EXPECT_EQ(report->recurring_donation, 4.4);
        EXPECT_EQ(report->one_time_donation, 4.5);
      }));

  rewards_service_->GetBalanceReport(
      5,
      2020,
      base::BindLambdaForTesting(
          [&](
              const ledger::type::Result result,
              ledger::type::BalanceReportInfoPtr report) {
        EXPECT_EQ(report->grants, 5.1);
        EXPECT_EQ(report->earning_from_ads, 5.2);
        EXPECT_EQ(report->auto_contribute, 5.3);
        EXPECT_EQ(report->recurring_donation, 5.4);
        EXPECT_EQ(report->one_time_donation, 5.5);
      }));
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, State_2) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", -1);
  rewards_browsertest_util::StartProcess(rewards_service_);
  const std::string wallet = R"({"payment_id":"eea767c4-cd27-4411-afd4-78a9c6b54dbc","recovery_seed":"PgFfhazUJuf8dX+8ckTjrtK1KMLyrfXmKJFDiS1Ad3I="})";  // NOLINT
  EXPECT_EQ(
      rewards_service_->GetEncryptedStringState("wallets.brave"),
      wallet);
  EXPECT_EQ(
      profile_->GetPrefs()->GetUint64("brave.rewards.creation_stamp"),
      1590484778ul);
  EXPECT_EQ(
      profile_->GetPrefs()->GetUint64("brave.rewards.ac.next_reconcile_stamp"),
      2593076778ul);
  EXPECT_EQ(
      profile_->GetPrefs()->GetDouble("brave.rewards.ac.amount"),
      20.0);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      true);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.inline_tip.reddit"),
      true);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.inline_tip.twitter"),
      false);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.inline_tip.github"),
      false);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsEnabledACEnabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", -1);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", true);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", true);
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      true);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsEnabledACDisabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", -1);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", true);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", false);
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      false);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsDisabledACEnabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", -1);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", false);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", true);
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      false);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsDisabledACDisabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", -1);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", false);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", false);
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      false);
}

}  // namespace rewards_browsertest
