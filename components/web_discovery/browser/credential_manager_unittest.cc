/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/credential_manager.h"

#include <memory>
#include <string>
#include <utility>

#include "base/base64.h"
#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/util.h"
#include "brave/components/web_discovery/browser/web_discovery_service.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

class WebDiscoveryCredentialManagerTest : public testing::Test {
 public:
  WebDiscoveryCredentialManagerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  ~WebDiscoveryCredentialManagerTest() override = default;

  // testing::Test:
  void SetUp() override {
    base::Time set_time;
    ASSERT_TRUE(base::Time::FromUTCString("2024-06-22", &set_time));
    task_environment_.AdvanceClock(set_time - base::Time::Now());

    WebDiscoveryService::RegisterProfilePrefs(profile_prefs_.registry());

    base::FilePath data_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    std::string test_data_json;
    ASSERT_TRUE(base::ReadFileToString(
        data_path.AppendASCII(
            "web_discovery/credential_keys_and_responses.json"),
        &test_data_json));
    auto test_data_dict = base::test::ParseJsonDict(test_data_json);
    const auto* rsa_priv_key = test_data_dict.FindString("rsa_priv_key");
    const auto* group_pub_key = test_data_dict.FindString("group_pub_key");
    const auto* join_responses = test_data_dict.FindDict("join_responses");
    ASSERT_TRUE(rsa_priv_key && group_pub_key && join_responses);

    profile_prefs_.SetString(kCredentialRSAPrivateKey, *rsa_priv_key);

    server_config_loader_ = std::make_unique<ServerConfigLoader>(
        nullptr, base::FilePath(), nullptr, base::DoNothing(),
        base::DoNothing());

    auto server_config = std::make_unique<ServerConfig>();
    for (const auto [date, join_response] : *join_responses) {
      auto decoded_group_pub_key = base::Base64Decode(*group_pub_key);
      ASSERT_TRUE(decoded_group_pub_key);
      server_config->group_pub_keys[date] = *decoded_group_pub_key;
      join_responses_[date] = join_response.GetString();
    }
    server_config_loader_->SetLastServerConfigForTesting(
        std::move(server_config));

    url_loader_factory_.SetInterceptor(
        base::BindRepeating(&WebDiscoveryCredentialManagerTest::HandleRequest,
                            base::Unretained(this)));

    SetUpCredentialManager();
  }

 protected:
  void SetUpCredentialManager() {
    credential_manager_ = std::make_unique<CredentialManager>(
        &profile_prefs_, shared_url_loader_factory_.get(),
        server_config_loader_.get());
    credential_manager_->UseFixedSeedForTesting();
  }

  void HandleRequest(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();
    std::string response;
    const auto* elements = request.request_body->elements();
    ASSERT_EQ(elements->size(), 1u);
    ASSERT_EQ(elements->at(0).type(), network::DataElement::Tag::kBytes);
    auto body_json =
        elements->at(0).As<network::DataElementBytes>().AsStringPiece();

    auto body_value = base::test::ParseJsonDict(body_json);
    const auto* ts = body_value.FindString("ts");
    ASSERT_TRUE(ts);
    ASSERT_TRUE(join_responses_.contains(*ts));
    ASSERT_EQ(request.url.spec(), GetDirectHPNHost() + "/join");

    base::DictValue dict;
    dict.Set("joinResponse", join_responses_.at(*ts));
    ASSERT_TRUE(
        base::JSONWriter::Write(base::Value(std::move(dict)), &response));
    url_loader_factory_.AddResponse(request.url.spec(), response);
    join_requests_made_++;
  }

  base::test::TaskEnvironment task_environment_;

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  TestingPrefServiceSimple profile_prefs_;
  std::unique_ptr<ServerConfigLoader> server_config_loader_;

  base::flat_map<std::string, std::string> join_responses_;

  std::unique_ptr<CredentialManager> credential_manager_;

  size_t join_requests_made_ = 0;
};

TEST_F(WebDiscoveryCredentialManagerTest, JoinGroups) {
  credential_manager_->JoinGroups();
  task_environment_.RunUntilIdle();

  EXPECT_EQ(join_requests_made_, 3u);
  join_requests_made_ = 0;

  for (size_t i = 0; i < 3; i++) {
    EXPECT_TRUE(credential_manager_->CredentialExistsForToday());
    task_environment_.FastForwardBy(base::Days(1));
  }
  EXPECT_FALSE(credential_manager_->CredentialExistsForToday());
  EXPECT_EQ(join_requests_made_, 0u);
}

TEST_F(WebDiscoveryCredentialManagerTest, LoadKeysFromStorage) {
  credential_manager_->JoinGroups();
  task_environment_.RunUntilIdle();

  EXPECT_EQ(join_requests_made_, 3u);
  join_requests_made_ = 0;

  SetUpCredentialManager();
  for (size_t i = 0; i < 3; i++) {
    ASSERT_TRUE(credential_manager_->CredentialExistsForToday());
    task_environment_.FastForwardBy(base::Days(1));
  }
  ASSERT_FALSE(credential_manager_->CredentialExistsForToday());
  EXPECT_EQ(join_requests_made_, 0u);
}

TEST_F(WebDiscoveryCredentialManagerTest, Sign) {
  std::vector<uint8_t> message({0, 1, 2, 3, 4});
  std::vector<uint8_t> basename({5, 6, 7, 8, 9});
  credential_manager_->Sign(
      message, basename,
      base::BindLambdaForTesting(
          [&](const std::optional<std::vector<uint8_t>> signature) {
            EXPECT_FALSE(signature);
          }));
  task_environment_.RunUntilIdle();

  credential_manager_->JoinGroups();
  task_environment_.RunUntilIdle();

  base::flat_set<std::vector<uint8_t>> signatures;
  for (size_t i = 0; i < 3; i++) {
    credential_manager_->Sign(
        message, basename,
        base::BindLambdaForTesting(
            [&](const std::optional<std::vector<uint8_t>> signature) {
              ASSERT_TRUE(signature);
              EXPECT_FALSE(signature->empty());
              EXPECT_FALSE(signatures.contains(*signature));
              signatures.insert(*signature);
            }));
    task_environment_.FastForwardBy(base::Days(1));
  }
  credential_manager_->Sign(
      message, basename,
      base::BindLambdaForTesting(
          [&](const std::optional<std::vector<uint8_t>> signature) {
            EXPECT_FALSE(signature);
          }));
  task_environment_.RunUntilIdle();
}

}  // namespace web_discovery
