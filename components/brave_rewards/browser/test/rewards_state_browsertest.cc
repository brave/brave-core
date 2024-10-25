/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <optional>
#include <set>

#include "base/base64.h"
#include "base/base64url.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_network_util.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_response.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_test.h"
#include "crypto/sha2.h"
#include "net/dns/mock_host_resolver.h"

// npm run test -- brave_browser_tests --filter=RewardsStateBrowserTest*

namespace brave_rewards {

class RewardsStateBrowserTest : public InProcessBrowserTest {
 public:
  RewardsStateBrowserTest() {
    response_ = std::make_unique<test_util::RewardsBrowserTestResponse>();
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
    https_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::test_server::EmbeddedTestServer::TYPE_HTTPS);
    https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_->RegisterRequestHandler(
        base::BindRepeating(&test_util::HandleRequest));
    ASSERT_TRUE(https_server_->Start());

    // Rewards service
    profile_ = browser()->profile();
    rewards_service_ = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetForProfile(profile_));

    // Response mock
    base::ScopedAllowBlockingForTesting allow_blocking;
    response_->LoadMocks();
    rewards_service_->ForTestingSetTestResponseCallback(
        base::BindRepeating(
            &RewardsStateBrowserTest::GetTestResponse,
            base::Unretained(this)));
    rewards_service_->SetEngineEnvForTesting();
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

    int32_t test_version = 0;
    CHECK(base::StringToInt(version_split[1], &test_version));

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
    base::FilePath test_path =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);
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

  raw_ptr<RewardsServiceImpl> rewards_service_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
  std::unique_ptr<net::EmbeddedTestServer> https_server_;
  std::unique_ptr<test_util::RewardsBrowserTestResponse> response_;
};

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, State_1) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", -1);
  test_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetInteger("brave.rewards.ac.min_visit_time"),
      5);
  EXPECT_EQ(profile_->GetPrefs()->GetInteger("brave.rewards.ac.min_visits"), 5);
  EXPECT_EQ(profile_->GetPrefs()->GetDouble("brave.rewards.ac.score.a"),
      14500.0);
  EXPECT_EQ(profile_->GetPrefs()->GetDouble("brave.rewards.ac.score.b"),
      -14000.0);

  rewards_service_->GetBalanceReport(
      4, 2020,
      base::BindLambdaForTesting(
          [&](const mojom::Result result, mojom::BalanceReportInfoPtr report) {
            ASSERT_TRUE(report);
            EXPECT_EQ(report->grants, 4.1);
            EXPECT_EQ(report->earning_from_ads, 4.2);
            EXPECT_EQ(report->auto_contribute, 4.3);
            EXPECT_EQ(report->recurring_donation, 4.4);
            EXPECT_EQ(report->one_time_donation, 4.5);
          }));

  rewards_service_->GetBalanceReport(
      5, 2020,
      base::BindLambdaForTesting(
          [&](const mojom::Result result, mojom::BalanceReportInfoPtr report) {
            ASSERT_TRUE(report);
            EXPECT_EQ(report->grants, 5.1);
            EXPECT_EQ(report->earning_from_ads, 5.2);
            EXPECT_EQ(report->auto_contribute, 5.3);
            EXPECT_EQ(report->recurring_donation, 5.4);
            EXPECT_EQ(report->one_time_donation, 5.5);
          }));
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, State_2) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 1);

  test_util::StartProcess(rewards_service_);

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
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsEnabledACEnabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 7);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", true);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", true);
  test_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      true);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsEnabledACDisabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 7);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", true);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", false);
  test_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      false);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsDisabledACEnabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 1);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", false);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", true);
  test_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      false);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V8RewardsDisabledACDisabled) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 7);
  profile_->GetPrefs()->SetBoolean("brave.rewards.enabled", false);
  profile_->GetPrefs()->SetBoolean("brave.rewards.ac.enabled", false);
  test_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetBoolean("brave.rewards.ac.enabled"),
      false);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V11ValidWallet) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 10);

  const std::string wallet = "wallet";

  const auto encrypted = test_util::EncryptPrefString(wallet);
  ASSERT_TRUE(encrypted);
  profile_->GetPrefs()->SetString("brave.rewards.wallets.brave", *encrypted);

  test_util::StartProcess(rewards_service_);

  const auto brave_wallet =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.brave");

  EXPECT_EQ(brave_wallet, wallet);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V11CorruptedWallet) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 10);
  profile_->GetPrefs()->SetString("brave.rewards.wallets.brave",
                                  base::Base64Encode("foobar"));

  test_util::StartProcess(rewards_service_);

  const auto brave_wallet =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.brave");
  const auto decrypted = test_util::DecryptPrefString(brave_wallet);

  EXPECT_FALSE(decrypted);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V11InvalidWallet) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 10);

  profile_->GetPrefs()->SetString("brave.rewards.wallets.brave", "foobar");

  test_util::StartProcess(rewards_service_);

  const auto brave_wallet =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.brave");
  const auto decrypted = test_util::DecryptPrefString(brave_wallet);

  EXPECT_FALSE(decrypted);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V11EmptyWallet) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 10);

  test_util::StartProcess(rewards_service_);

  const auto brave_wallet =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.brave");

  EXPECT_TRUE(brave_wallet.empty());
}

class V10 : public RewardsStateBrowserTest,
            public ::testing::WithParamInterface<
                std::pair<std::string, std::string>> {
 public:
  static std::string NameSuffixGenerator(
      const ::testing::TestParamInfo<V10::ParamType>& info) {
    return from_json(std::get<0>(info.param)) + "__" +
           from_json(std::get<1>(info.param));
  }

 private:
  static std::string from_json(const std::string& json) {
    std::string suffix = "";

    std::optional<base::Value> value = base::JSONReader::Read(json);
    if (value && value->is_dict()) {
      const auto& dict = value->GetDict();
      suffix += to_string(dict.FindInt("status").value_or(-1));
      suffix += to_string("token", dict.FindString("token"));
      suffix += to_string("address", dict.FindString("address"));
    }

    return suffix;
  }

  static std::string to_string(int status) {
    switch (status) {
      case 0:
        return "NOT_CONNECTED";
      case 1:
        return "CONNECTED";
      case 2:
        return "VERIFIED";
      case 3:
        return "DISCONNECTED_NOT_VERIFIED";
      case 4:
        return "DISCONNECTED_VERIFIED";
      case 5:
        return "PENDING";
      default:
        return "unknown_WalletStatus_value";
    }
  }

  static std::string to_string(const std::string& key,
                               const std::string* value) {
    std::string suffix{'_' + key};

    if (value) {
      suffix += '_' + std::string{value->empty() ? "empty" : "non_empty"};
    }

    return suffix;
  }
};

#ifdef OFFICIAL_BUILD
#define _UPHOLD_CLIENT_ID_ \
  +std::string(BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_ID)) +
#define _UPHOLD_URL_ +std::string(BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL)) +
#else
#define _UPHOLD_CLIENT_ID_ +std::string(BUILDFLAG(UPHOLD_SANDBOX_CLIENT_ID)) +
#define _UPHOLD_URL_ +std::string(BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL)) +
#endif

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    RewardsStateBrowserTest,
    V10,
    ::testing::Values(
        // NOLINTNEXTLINE
        std::make_pair(  // NOT_CONNECTED_token_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // NOT_CONNECTED_token_non_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // NOT_CONNECTED_token_empty_address_non_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // NOT_CONNECTED_token_non_empty_address_non_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // CONNECTED_token_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":1,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // CONNECTED_token_non_empty_address_empty__PENDING_token_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":1,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // CONNECTED_token_empty_address_non_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":1,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // CONNECTED_token_non_empty_address_non_empty__PENDING_token_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":1,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // VERIFIED_token_empty_address_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":2,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // VERIFIED_token_non_empty_address_empty__PENDING_token_non_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":2,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // VERIFIED_token_empty_address_non_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":2,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // VERIFIED_token_non_empty_address_non_empty__PENDING_token_non_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":2,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_NOT_VERIFIED_token_empty_address_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":3,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_NOT_VERIFIED_token_non_empty_address_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":3,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_NOT_VERIFIED_token_empty_address_non_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":3,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_NOT_VERIFIED_token_non_empty_address_non_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":3,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_VERIFIED_token_empty_address_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_VERIFIED_token_non_empty_address_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_VERIFIED_token_empty_address_non_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // DISCONNECTED_VERIFIED_token_non_empty_address_non_empty__DISCONNECTED_VERIFIED_token_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":4,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // PENDING_token_empty_address_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":5,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // PENDING_token_non_empty_address_empty__PENDING_token_non_empty_address_empty
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // PENDING_token_empty_address_non_empty__NOT_CONNECTED_token_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":5,)"
                R"("token":"",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":0,)"
                R"("token":"",)"
                R"("user_name":""})"}),
        // NOLINTNEXTLINE
        std::make_pair(  // PENDING_token_non_empty_address_non_empty__PENDING_token_non_empty_address_empty
            std::string{
                R"({"address":"962df5b1-bb72-4619-a349-c8087941b795",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"},
            std::string{
                R"({"address":"",)"
                R"("fees":{},)"
                R"("member_id":"",)"
                R"("status":5,)"
                R"("token":"0047c2fd8f023e067354dbdb5639ee67acf77150",)"
                R"("user_name":""})"})),
    V10::NameSuffixGenerator);
// clang-format on

IN_PROC_BROWSER_TEST_P_(V10, Paths) {
  // testing migration from v9 to v10
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 9);
  rewards_service_->SetEngineStateTargetVersionForTesting(10);

  const auto& params = GetParam();
  const auto& from_wallet = std::get<0>(params);
  const auto& expected_wallet = std::get<1>(params);

  const auto encrypted_from_wallet = test_util::EncryptPrefString(from_wallet);
  ASSERT_TRUE(encrypted_from_wallet);
  profile_->GetPrefs()->SetString("brave.rewards.wallets.uphold",
                                  *encrypted_from_wallet);

  test_util::StartProcess(rewards_service_);

  const auto encrypted_to_wallet =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.uphold");
  const auto decrypted_to_wallet =
      test_util::DecryptPrefString(encrypted_to_wallet);
  ASSERT_TRUE(decrypted_to_wallet);

  EXPECT_EQ(*decrypted_to_wallet, expected_wallet);
}

class V12 : public RewardsStateBrowserTest,
            public testing::WithParamInterface<
                std::tuple<std::string, std::string, mojom::WalletStatus>> {};

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  RewardsStateBrowserTest,
  V12,
  testing::Values(
    std::tuple<std::string, std::string, mojom::WalletStatus>(
      "pre_v12_NOT_CONNECTED__v12_kNotConnected",
      R"(
        {
          "status": 0,
          "token": "token",
          "address": "address"
        }
      )",
      mojom::WalletStatus::kNotConnected
    ),
    std::tuple<std::string, std::string, mojom::WalletStatus>(
      "pre_v12_VERIFIED__v12_kConnected",
      R"(
        {
          "status": 2,
          "token": "token",
          "address": "address"
        }
      )",
      mojom::WalletStatus::kConnected
    ),
    std::tuple<std::string, std::string, mojom::WalletStatus>(
      "pre_v12_VERIFIED__v12_kLoggedOut",
      R"(
        {
          "status": 2,
          "token": "",
          "address": "address"
        }
      )",
      mojom::WalletStatus::kLoggedOut
    ),
    std::tuple<std::string, std::string, mojom::WalletStatus>(
      "pre_v12_DISCONNECTED_VERIFIED_v12_VERIFIED__kLoggedOut",
      R"(
        {
          "status": 4,
          "token": "token",
          "address": "address"
        }
      )",
      mojom::WalletStatus::kLoggedOut
    ),
    std::tuple<std::string, std::string, mojom::WalletStatus>(
      "pre_v12_CONNECTED__v12_kNotConnected",
      R"(
        {
          "status": 1,
          "token": "token",
          "address": "address"
        }
      )",
      mojom::WalletStatus::kNotConnected
    ),
    std::tuple<std::string, std::string, mojom::WalletStatus>(
      "pre_v12_DISCONNECTED_NOT_VERIFIED__v12_kNotConnected",
      R"(
        {
          "status": 3,
          "token": "token",
          "address": "address"
        }
      )",
      mojom::WalletStatus::kNotConnected
    ),
    std::tuple<std::string, std::string, mojom::WalletStatus>(
      "pre_v12_PENDING__v12_kNotConnected",
      R"(
        {
          "status": 5,
          "token": "token",
          "address": "address"
        }
      )",
      mojom::WalletStatus::kNotConnected
    )
  ),
  [](const auto& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

IN_PROC_BROWSER_TEST_P_(V12, Paths) {
  // testing migration from v11 to v12
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 11);
  rewards_service_->SetEngineStateTargetVersionForTesting(12);

  const auto encrypted_from_wallet =
      test_util::EncryptPrefString(std::get<1>(GetParam()));
  ASSERT_TRUE(encrypted_from_wallet);
  profile_->GetPrefs()->SetString("brave.rewards.wallets.bitflyer",
                                  *encrypted_from_wallet);

  test_util::StartProcess(rewards_service_);

  const auto encrypted_to_wallet =
      profile_->GetPrefs()->GetString("brave.rewards.wallets.bitflyer");
  const auto decrypted_to_wallet =
      test_util::DecryptPrefString(encrypted_to_wallet);
  ASSERT_TRUE(decrypted_to_wallet);

  const auto value = base::JSONReader::Read(*decrypted_to_wallet);
  ASSERT_TRUE(value && value->is_dict());

  const auto& wallet_dict = value->GetDict();

  const auto status = wallet_dict.FindInt("status");
  const auto* token = wallet_dict.FindString("token");
  const auto* address = wallet_dict.FindString("address");

  ASSERT_TRUE(status && token && address);

  ASSERT_TRUE(
      (std::set{0 /* kNotConnected */, 2 /* kConnected */, 4 /* kLoggedOut */}
           .count(*status)));
  ASSERT_TRUE(static_cast<mojom::WalletStatus>(*status) ==
              std::get<2>(GetParam()));

  if (*status == 0 /* kNotConnected */ || *status == 4 /* kLoggedOut */) {
    ASSERT_TRUE(token->empty());
    ASSERT_TRUE(address->empty());
  } else {  // *status == 2 /* kConnected */
    ASSERT_FALSE(token->empty());
    ASSERT_FALSE(address->empty());
  }
}

class V13 : public RewardsStateBrowserTest,
            public testing::WithParamInterface<
                std::tuple<std::string, mojom::WalletStatus>> {};

INSTANTIATE_TEST_SUITE_P(
    RewardsStateBrowserTest,
    V13,
    testing::Combine(testing::Values(std::string("bitflyer"),
                                     std::string("gemini"),
                                     std::string("uphold")),
                     testing::Values(mojom::WalletStatus::kNotConnected,
                                     mojom::WalletStatus::kConnected,
                                     mojom::WalletStatus::kLoggedOut)),
    [](const testing::TestParamInfo<V13::ParamType>& info) {
      return (std::ostringstream{} << std::get<0>(info.param) << '_'
                                   << std::get<1>(info.param) << '_'
                                   << static_cast<int>(std::get<1>(info.param)))
          .str();
    });

IN_PROC_BROWSER_TEST_P_(V13, Paths) {
  // testing migration from v12 to v13
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 12);
  rewards_service_->SetEngineStateTargetVersionForTesting(13);

  const auto wallet_status = std::get<1>(GetParam());
  const auto encrypted_wallet = test_util::EncryptPrefString(
      (std::ostringstream{} << R"({ "status": )"
                            << static_cast<int>(wallet_status) << " }")
          .str());
  ASSERT_TRUE(encrypted_wallet);
  profile_->GetPrefs()->SetString(
      "brave.rewards.wallets." + std::get<0>(GetParam()), *encrypted_wallet);
}

IN_PROC_BROWSER_TEST_F(RewardsStateBrowserTest, V14EmptyWalletType) {
  profile_->GetPrefs()->SetInteger("brave.rewards.version", 13);
  rewards_service_->SetEngineStateTargetVersionForTesting(14);

  auto store_wallet = [&](const std::string& key, const std::string& json) {
    auto encrypted_wallet = test_util::EncryptPrefString(json);
    profile_->GetPrefs()->SetString(key, *encrypted_wallet);
  };

  store_wallet("brave.rewards.wallets.gemini", R"({ "status": 0 })");
  store_wallet("brave.rewards.wallets.uphold", R"({ "status": 2 })");

  test_util::StartProcess(rewards_service_);
  EXPECT_EQ(
      profile_->GetPrefs()->GetString("brave.rewards.external_wallet_type"),
      "uphold");
}

}  // namespace brave_rewards
