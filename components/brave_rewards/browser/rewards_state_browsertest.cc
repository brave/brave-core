/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind_test_util.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_browser_tests --filter=RewardsStateBrowserTest.*

class RewardsStateBrowserTest
    : public InProcessBrowserTest,
      public brave_rewards::RewardsServiceObserver,
      public base::SupportsWeakPtr<BraveRewardsBrowserTest> {
 public:
  RewardsStateBrowserTest() {
  }

  ~RewardsStateBrowserTest() override {
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

    brave::RegisterPathProvider();

    profile_ = browser()->profile();

    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile_));

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

  std::unique_ptr<base::RunLoop> wait_for_wallet_initialization_loop_;
  bool wallet_initialized_ = false;
  Profile* profile_;
};

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, State_1) {
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
          [&](const int result, const brave_rewards::BalanceReport& report) {
        EXPECT_EQ(report.grants, 4.1);
        EXPECT_EQ(report.earning_from_ads, 4.2);
        EXPECT_EQ(report.auto_contribute, 4.3);
        EXPECT_EQ(report.recurring_donation, 4.4);
        EXPECT_EQ(report.one_time_donation, 4.5);
      }));

  rewards_service_->GetBalanceReport(
      5,
      2020,
      base::BindLambdaForTesting(
          [&](const int result, const brave_rewards::BalanceReport& report) {
        EXPECT_EQ(report.grants, 5.1);
        EXPECT_EQ(report.earning_from_ads, 5.2);
        EXPECT_EQ(report.auto_contribute, 5.3);
        EXPECT_EQ(report.recurring_donation, 5.4);
        EXPECT_EQ(report.one_time_donation, 5.5);
      }));
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, State_2) {
  EXPECT_EQ(
      profile_->GetPrefs()->GetString("brave.rewards.wallet.payment_id"),
      "eea767c4-cd27-4411-afd4-78a9c6b54dbc");
  EXPECT_EQ(
      profile_->GetPrefs()->GetString("brave.rewards.wallet.seed"),
      "PgFfhazUJuf8dX+8ckTjrtK1KMLyrfXmKJFDiS1Ad3I=");
  EXPECT_EQ(
      profile_->GetPrefs()->GetString("brave.rewards.wallet.anonymous_card_id"),
      "cf5b388c-eea2-4c98-bec2-f8daf39881a4");
  EXPECT_EQ(
      profile_->GetPrefs()->GetUint64("brave.rewards.creation_stamp"),
      1590484778ul);
  EXPECT_EQ(
      profile_->GetPrefs()->GetUint64("brave.rewards.ac.next_reconcile_stamp"),
      1593076778ul);
  EXPECT_EQ(
      profile_->GetPrefs()->GetDouble("brave.rewards.ac.amount"),
      20.0);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.enabled"),
      true);
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
