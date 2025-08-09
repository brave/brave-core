/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/server_config_loader.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/web_discovery/browser/util.h"
#include "brave/components/web_discovery/browser/web_discovery_service.h"
#include "brave/components/web_discovery/common/features.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

class WebDiscoveryServerConfigLoaderTest : public testing::Test {
 public:
  WebDiscoveryServerConfigLoaderTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  ~WebDiscoveryServerConfigLoaderTest() override = default;

  // testing::Test:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        web_discovery::features::kBraveWebDiscoveryNative,
        {{web_discovery::features::kPatternsVersionParam, "1"}});
    WebDiscoveryService::RegisterLocalStatePrefs(local_state_.registry());

    ASSERT_TRUE(install_dir_.CreateUniqueTempDir());

    base::FilePath data_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA)
            .AppendASCII("web_discovery");
    ASSERT_TRUE(base::ReadFileToString(data_path.AppendASCII("hpn-config.json"),
                                       &hpn_config_contents_));
    ASSERT_TRUE(base::ReadFileToString(
        data_path.AppendASCII("quorum-config.json"), &quorum_config_contents_));
    ASSERT_TRUE(base::ReadFileToString(data_path.AppendASCII("patterns.gz"),
                                       &patterns_gz_contents_));

    url_loader_factory_.SetInterceptor(
        base::BindRepeating(&WebDiscoveryServerConfigLoaderTest::HandleRequest,
                            base::Unretained(this)));
    SetupServerConfigLoader();
  }

 protected:
  void SetupServerConfigLoader() {
    server_config_loader_ = std::make_unique<ServerConfigLoader>(
        &local_state_, install_dir_.GetPath(), shared_url_loader_factory_.get(),
        base::BindRepeating(
            &WebDiscoveryServerConfigLoaderTest::HandleConfigReady,
            base::Unretained(this)),
        base::BindRepeating(
            &WebDiscoveryServerConfigLoaderTest::HandlePatternsReady,
            base::Unretained(this)));
  }

  void ResetCounts() {
    hpn_config_requests_made_ = 0;
    quorum_config_requests_made_ = 0;
    patterns_requests_made_ = 0;
    config_ready_calls_made_ = 0;
    patterns_ready_calls_made_ = 0;
  }

  bool PatternsFileExists() {
    return base::PathExists(
        install_dir_.GetPath().AppendASCII("wdp_patterns.json"));
  }

  void HandleRequest(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();

    ASSERT_EQ(request.method, net::HttpRequestHeaders::kGetMethod);
    if (request.url.spec().starts_with(GetAnonymousHPNHost() + "/config")) {
      hpn_config_requests_made_++;
      url_loader_factory_.AddResponse(request.url.spec(), hpn_config_contents_,
                                      hpn_config_status_code_);
    } else if (request.url.spec() == GetQuorumHost() + "/config") {
      quorum_config_requests_made_++;
      url_loader_factory_.AddResponse(request.url.spec(),
                                      quorum_config_contents_,
                                      quorum_config_status_code_);
    } else if (request.url.spec() == GetPatternsEndpoint()) {
      patterns_requests_made_++;
      url_loader_factory_.AddResponse(request.url.spec(), patterns_gz_contents_,
                                      patterns_status_code_);
    } else {
      FAIL();
    }
  }

  void HandleConfigReady() { config_ready_calls_made_++; }

  void HandlePatternsReady() { patterns_ready_calls_made_++; }

  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  TestingPrefServiceSimple local_state_;

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  base::ScopedTempDir install_dir_;

  size_t hpn_config_requests_made_ = 0;
  size_t quorum_config_requests_made_ = 0;
  size_t patterns_requests_made_ = 0;

  size_t config_ready_calls_made_ = 0;
  size_t patterns_ready_calls_made_ = 0;

  net::HttpStatusCode hpn_config_status_code_ = net::HTTP_OK;
  net::HttpStatusCode quorum_config_status_code_ = net::HTTP_OK;
  net::HttpStatusCode patterns_status_code_ = net::HTTP_OK;

  std::string patterns_gz_contents_;
  std::string quorum_config_contents_;
  std::string hpn_config_contents_;

  std::unique_ptr<ServerConfigLoader> server_config_loader_;
};

TEST_F(WebDiscoveryServerConfigLoaderTest, LoadConfigs) {
  EXPECT_EQ(hpn_config_requests_made_, 0u);
  EXPECT_EQ(quorum_config_requests_made_, 0u);
  EXPECT_EQ(patterns_requests_made_, 0u);
  EXPECT_EQ(config_ready_calls_made_, 0u);
  EXPECT_EQ(patterns_ready_calls_made_, 0u);

  task_environment_.FastForwardBy(base::Seconds(40));

  server_config_loader_->LoadConfigs();
  task_environment_.RunUntilIdle();

  EXPECT_EQ(hpn_config_requests_made_, 1u);
  EXPECT_EQ(quorum_config_requests_made_, 1u);
  EXPECT_EQ(patterns_requests_made_, 0u);
  EXPECT_EQ(config_ready_calls_made_, 1u);
  EXPECT_EQ(patterns_ready_calls_made_, 0u);

  EXPECT_FALSE(PatternsFileExists());

  const auto& server_config = server_config_loader_->GetLastServerConfig();
  EXPECT_EQ(server_config.location, "ca");
  EXPECT_EQ(server_config.group_pub_keys.size(), 4u);
  EXPECT_EQ(server_config.pub_keys.size(), 4u);
  EXPECT_EQ(server_config.source_map_actions.size(), 27u);

  task_environment_.FastForwardBy(base::Seconds(40));
  EXPECT_EQ(hpn_config_requests_made_, 1u);
  EXPECT_EQ(quorum_config_requests_made_, 1u);
  EXPECT_EQ(patterns_requests_made_, 1u);
  EXPECT_EQ(config_ready_calls_made_, 1u);
  EXPECT_EQ(patterns_ready_calls_made_, 1u);

  EXPECT_TRUE(PatternsFileExists());

  const auto& patterns = server_config_loader_->GetLastPatterns();
  EXPECT_EQ(patterns.normal_patterns.size(), 9u);
  EXPECT_EQ(patterns.strict_patterns.size(), 8u);
}

TEST_F(WebDiscoveryServerConfigLoaderTest, ReloadConfigs) {
  server_config_loader_->LoadConfigs();
  task_environment_.FastForwardBy(base::Seconds(40));

  for (size_t i = 0; i < 3; i++) {
    ResetCounts();
    task_environment_.FastForwardBy(base::Hours(4));

    EXPECT_GE(hpn_config_requests_made_, 1u);
    EXPECT_GE(quorum_config_requests_made_, 1u);
    EXPECT_GE(patterns_requests_made_, 1u);
    EXPECT_GE(config_ready_calls_made_, 1u);
    EXPECT_GE(patterns_ready_calls_made_, 1u);
    EXPECT_TRUE(PatternsFileExists());
  }
}

TEST_F(WebDiscoveryServerConfigLoaderTest, LoadPatternsFromStorage) {
  server_config_loader_->LoadConfigs();
  task_environment_.FastForwardBy(base::Seconds(40));
  EXPECT_TRUE(PatternsFileExists());

  ResetCounts();
  SetupServerConfigLoader();
  server_config_loader_->LoadConfigs();
  task_environment_.FastForwardBy(base::Seconds(40));

  EXPECT_EQ(hpn_config_requests_made_, 1u);
  EXPECT_EQ(quorum_config_requests_made_, 1u);
  EXPECT_EQ(patterns_requests_made_, 0u);
  EXPECT_EQ(config_ready_calls_made_, 1u);
  EXPECT_EQ(patterns_ready_calls_made_, 1u);
  EXPECT_TRUE(PatternsFileExists());

  task_environment_.AdvanceClock(base::Hours(3));
  ResetCounts();
  SetupServerConfigLoader();
  server_config_loader_->LoadConfigs();
  task_environment_.FastForwardBy(base::Seconds(40));

  EXPECT_EQ(hpn_config_requests_made_, 1u);
  EXPECT_EQ(quorum_config_requests_made_, 1u);
  EXPECT_EQ(patterns_requests_made_, 1u);
  EXPECT_EQ(config_ready_calls_made_, 1u);
  EXPECT_EQ(patterns_ready_calls_made_, 1u);
  EXPECT_TRUE(PatternsFileExists());
}

TEST_F(WebDiscoveryServerConfigLoaderTest, ConfigRetry) {
  hpn_config_status_code_ = net::HTTP_INTERNAL_SERVER_ERROR;
  server_config_loader_->LoadConfigs();
  task_environment_.FastForwardBy(base::Seconds(40));

  EXPECT_GE(hpn_config_requests_made_, 1u);
  EXPECT_GE(quorum_config_requests_made_, 1u);
  EXPECT_EQ(patterns_requests_made_, 0u);
  EXPECT_EQ(config_ready_calls_made_, 0u);
  EXPECT_EQ(patterns_ready_calls_made_, 0u);

  ResetCounts();
  hpn_config_status_code_ = net::HTTP_OK;
  quorum_config_status_code_ = net::HTTP_INTERNAL_SERVER_ERROR;
  task_environment_.FastForwardBy(base::Seconds(40));

  EXPECT_GE(hpn_config_requests_made_, 1u);
  EXPECT_GE(quorum_config_requests_made_, 1u);
  EXPECT_EQ(patterns_requests_made_, 0u);
  EXPECT_EQ(config_ready_calls_made_, 0u);
  EXPECT_EQ(patterns_ready_calls_made_, 0u);
  EXPECT_FALSE(PatternsFileExists());

  ResetCounts();
  quorum_config_status_code_ = net::HTTP_OK;
  task_environment_.FastForwardBy(base::Seconds(90));

  EXPECT_EQ(hpn_config_requests_made_, 1u);
  EXPECT_EQ(quorum_config_requests_made_, 1u);
  EXPECT_EQ(config_ready_calls_made_, 1u);

  task_environment_.FastForwardBy(base::Seconds(40));
  EXPECT_EQ(patterns_requests_made_, 1u);
  EXPECT_EQ(patterns_ready_calls_made_, 1u);
  EXPECT_TRUE(PatternsFileExists());
}

TEST_F(WebDiscoveryServerConfigLoaderTest, PatternsRetry) {
  patterns_status_code_ = net::HTTP_INTERNAL_SERVER_ERROR;
  server_config_loader_->LoadConfigs();
  task_environment_.FastForwardBy(base::Seconds(40));

  EXPECT_EQ(hpn_config_requests_made_, 1u);
  EXPECT_EQ(quorum_config_requests_made_, 1u);
  EXPECT_GE(patterns_requests_made_, 1u);
  EXPECT_EQ(config_ready_calls_made_, 1u);
  EXPECT_EQ(patterns_ready_calls_made_, 0u);
  EXPECT_FALSE(PatternsFileExists());

  ResetCounts();
  patterns_status_code_ = net::HTTP_OK;
  task_environment_.FastForwardBy(base::Seconds(40));

  EXPECT_EQ(hpn_config_requests_made_, 0u);
  EXPECT_EQ(quorum_config_requests_made_, 0u);
  EXPECT_EQ(patterns_requests_made_, 1u);
  EXPECT_EQ(config_ready_calls_made_, 0u);
  EXPECT_EQ(patterns_ready_calls_made_, 1u);
  EXPECT_TRUE(PatternsFileExists());
}

}  // namespace web_discovery
