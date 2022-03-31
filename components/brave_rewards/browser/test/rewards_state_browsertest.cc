/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/base64.h"
#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "bat/ledger/internal/state/state_keys.h"
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
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_browser_tests --filter=RewardsStateBrowserTest*

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

  raw_ptr<brave_rewards::RewardsServiceImpl> rewards_service_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
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
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 1);

  rewards_browsertest_util::StartProcess(rewards_service_);

  const std::string wallet_json =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.brave");
  EXPECT_EQ(
      wallet_json,
      R"({"payment_id":"eea767c4-cd27-4411-afd4-78a9c6b54dbc","recovery_seed":"PgFfhazUJuf8dX+8ckTjrtK1KMLyrfXmKJFDiS1Ad3I="})");  // NOLINT
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
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 7);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", true);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", true);
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      true);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsEnabledACDisabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 7);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", true);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", false);
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      false);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsDisabledACEnabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 1);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", false);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", true);
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      false);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsDisabledACDisabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 7);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", false);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", false);
  rewards_browsertest_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      false);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V11ValidWallet) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 10);

  const std::string wallet = "wallet";

  const auto encrypted =
      rewards_browsertest_util::EncryptPrefString(rewards_service_, wallet);
  ASSERT_TRUE(encrypted);
  profile_->GetPrefs()->SetString("brave.rewards.wallets.brave", *encrypted);

  rewards_browsertest_util::StartProcess(rewards_service_);

  const auto brave_wallet =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.brave");

  EXPECT_EQ(brave_wallet, wallet);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V11CorruptedWallet) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 10);

  std::string base64_wallet;
  base::Base64Encode("foobar", &base64_wallet);
  profile_->GetPrefs()->SetString("brave.rewards.wallets.brave", base64_wallet);

  rewards_browsertest_util::StartProcess(rewards_service_);

  const auto brave_wallet =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.brave");
  const auto decrypted = rewards_browsertest_util::DecryptPrefString(
      rewards_service_, brave_wallet);

  EXPECT_FALSE(decrypted);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V11InvalidWallet) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 10);

  profile_->GetPrefs()->SetString("brave.rewards.wallets.brave", "foobar");

  rewards_browsertest_util::StartProcess(rewards_service_);

  const auto brave_wallet =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.brave");
  const auto decrypted = rewards_browsertest_util::DecryptPrefString(
      rewards_service_, brave_wallet);

  EXPECT_FALSE(decrypted);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V11EmptyWallet) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 10);

  rewards_browsertest_util::StartProcess(rewards_service_);

  const auto brave_wallet =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.brave");

  EXPECT_TRUE(brave_wallet.empty());
}

class UpholdStateMachine : public RewardsStateBrowserTest,
                           public ::testing::WithParamInterface<
                               std::pair<std::string, std::string>> {
 public:
  static std::string NameSuffixGenerator(
      const ::testing::TestParamInfo<UpholdStateMachine::ParamType>& info) {
    return from_json(std::get<0>(info.param)) + "__" +
           from_json(std::get<1>(info.param));
  }

 private:
  static std::string from_json(const std::string& json) {
    std::string suffix = "";

    absl::optional<base::Value> value = base::JSONReader::Read(json);
    if (value && value->is_dict()) {
      base::DictionaryValue* dictionary = nullptr;
      if (value->GetAsDictionary(&dictionary)) {
        suffix += to_string(dictionary->FindIntKey("status").value_or(-1));
        suffix += to_string("token", dictionary->FindStringKey("token"));
        suffix += to_string("address", dictionary->FindStringKey("address"));
      }
    }

    return suffix;
  }

  static std::string to_string(int status) {
    return status == -1 ? "unknown_WalletStatus_value"
                        : (std::ostringstream{}
                           << static_cast<ledger::type::WalletStatus>(status))
                              .str();
  }

  static std::string to_string(const std::string& key, std::string* value) {
    std::string suffix{'_' + key};

    if (value) {
      suffix += '_' + std::string{value->empty() ? "empty" : "non_empty"};
    }

    return suffix;
  }
};

#ifdef OFFICIAL_BUILD
#define _UPHOLD_CLIENT_ID_ UPHOLD_CLIENT_ID
#define _UPHOLD_URL_ "https://uphold.com"
#else
#define _UPHOLD_CLIENT_ID_ UPHOLD_STAGING_CLIENT_ID
#define _UPHOLD_URL_ "https://wallet-sandbox.uphold.com"
#endif

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    RewardsStateBrowserTest,
    UpholdStateMachine,
    ::testing::Values(
        // NOLINTNEXTLINE
        std::make_pair(  // NOT_CONNECTED_token_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // NOT_CONNECTED_token_non_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // NOT_CONNECTED_token_empty_address_non_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // NOT_CONNECTED_token_non_empty_address_non_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // CONNECTED_token_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":1,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // CONNECTED_token_non_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":1,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("withdraw_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc"})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // CONNECTED_token_empty_address_non_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":1,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // CONNECTED_token_non_empty_address_non_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":1,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("withdraw_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc"})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // VERIFIED_token_empty_address_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":2,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // VERIFIED_token_non_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":2,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("withdraw_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc"})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // VERIFIED_token_empty_address_non_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":2,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // VERIFIED_token_non_empty_address_non_empty__PENDING_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":2,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("withdraw_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc"})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_NOT_VERIFIED_token_empty_address_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":3,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_NOT_VERIFIED_token_non_empty_address_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":3,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_NOT_VERIFIED_token_empty_address_non_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":3,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_NOT_VERIFIED_token_non_empty_address_non_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":3,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_VERIFIED_token_empty_address_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_VERIFIED_token_non_empty_address_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_VERIFIED_token_empty_address_non_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_VERIFIED_token_non_empty_address_non_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // PENDING_token_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":5,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // PENDING_token_non_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("withdraw_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc"})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // PENDING_token_empty_address_non_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":5,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":"",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=kyc&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("withdraw_url":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // PENDING_token_non_empty_address_non_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"account_url":"",)"
                R"("add_url":"",)"
                R"("address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":"",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":"",)"
                R"("withdraw_url":""})"},
            std::string{
                R"({"account_url":")" _UPHOLD_URL_ R"(/dashboard",)"
                R"("activity_url":"",)"
                R"("add_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("address":"",)"
                R"("code_verifier":"",)"
                R"("fees":{},)"
                R"("login_url":")" _UPHOLD_URL_ R"(/authorize/)" _UPHOLD_CLIENT_ID_ R"(?scope=accounts:read accounts:write cards:read cards:write user:read transactions:deposit transactions:read transactions:transfer:application transactions:transfer:others&intention=login&state=49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("member_id":"",)"
                R"("one_time_string":"49E52DEFFC7C3309C8BF807FB8E838911362837961464845DCF1E58B50886D3C",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":"",)"
                R"("verify_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc",)"
                R"("withdraw_url":")" _UPHOLD_URL_ R"(/signup/step2?application_id=)" _UPHOLD_CLIENT_ID_ R"(&intention=kyc"})"})),
    UpholdStateMachine::NameSuffixGenerator);
// clang-format on

IN_PROC_BROWSER_TEST_P_(UpholdStateMachine, Migration) {
  using ledger::state::kWalletUphold;

  const auto& params = GetParam();
  const auto& from = std::get<0>(params);
  const auto& to = std::get<1>(params);

  profile_->GetPrefs()->SetInteger("brave.rewards.version", 9);
  auto encrypted =
      rewards_browsertest_util::EncryptPrefString(rewards_service_, from);
  ASSERT_TRUE(encrypted);
  profile_->GetPrefs()->SetString("brave.rewards.wallets.uphold", *encrypted);

  rewards_browsertest_util::StartProcess(rewards_service_);

  const auto uphold_pref =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.uphold");
  auto decrypted = rewards_browsertest_util::DecryptPrefString(rewards_service_,
                                                               uphold_pref);
  ASSERT_TRUE(decrypted);
  EXPECT_EQ(*decrypted, to);
}

}  // namespace rewards_browsertest
