/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/web_discovery_service.h"

#include <memory>
#include <string>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/weak_ptr.h"
#include "base/path_service.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "brave/components/web_discovery/browser/util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

class WebDiscoveryServiceTest : public testing::Test {
 private:
  class MockBackupResultsService : public brave_search::BackupResultsService {
   public:
    MockBackupResultsService() = default;
    ~MockBackupResultsService() override = default;

    // Override virtual methods with empty implementations
    void FetchBackupResults(const GURL& url,
                            std::optional<net::HttpRequestHeaders> headers,
                            BackupResultsCallback callback,
                            bool low_latency_required = false) override {}

    bool HandleWebContentsStartRequest(const content::WebContents* web_contents,
                                       const GURL& url) override {
      return true;
    }

    base::WeakPtr<BackupResultsService> GetWeakPtr() override {
      return nullptr;
    }
  };

 public:
  WebDiscoveryServiceTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  ~WebDiscoveryServiceTest() override = default;

  void SetUp() override {
    WebDiscoveryService::RegisterLocalStatePrefs(local_state_.registry());
    WebDiscoveryService::RegisterProfilePrefs(profile_prefs_.registry());
    profile_prefs_.registry()->RegisterBooleanPref(kWebDiscoveryEnabled, false);

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    // Only the last group in hpn-config.json is still eligible to join.
    base::Time last_group_date;
    ASSERT_TRUE(base::Time::FromUTCString("2024-06-27", &last_group_date));
    task_environment_.AdvanceClock(last_group_date - base::Time::Now());
  }

 protected:
  void CreateService() {
    service_ = std::make_unique<WebDiscoveryService>(
        &local_state_, &profile_prefs_, temp_dir_.GetPath(),
        shared_url_loader_factory_, &backup_results_service_);
  }

  void ExpectCredentialJoin() {
    const base::FilePath data_path =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
            .AppendASCII("brave/test/data/web_discovery");
    std::string collector_config;
    std::string quorum_config;
    ASSERT_TRUE(base::ReadFileToString(data_path.AppendASCII("hpn-config.json"),
                                       &collector_config));
    ASSERT_TRUE(base::ReadFileToString(
        data_path.AppendASCII("quorum-config.json"), &quorum_config));
    ASSERT_TRUE(url_loader_factory_.SimulateResponseForPendingRequest(
        GetAnonymousHPNHost() + "/config", collector_config, net::HTTP_OK,
        network::TestURLLoaderFactory::kUrlMatchPrefix));
    ASSERT_TRUE(url_loader_factory_.SimulateResponseForPendingRequest(
        GetQuorumHost() + "/config", quorum_config));
    url_loader_factory_.WaitForRequest(GURL(GetDirectHPNHost() + "/join"));
    EXPECT_TRUE(url_loader_factory_.IsPending(GetDirectHPNHost() + "/join"));
  }

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  TestingPrefServiceSimple profile_prefs_;

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  base::ScopedTempDir temp_dir_;
  MockBackupResultsService backup_results_service_;
  std::unique_ptr<WebDiscoveryService> service_;
};

TEST_F(WebDiscoveryServiceTest, ServiceStartsWhenEnabled) {
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, true);

  CreateService();

  ASSERT_NO_FATAL_FAILURE(ExpectCredentialJoin());
}

TEST_F(WebDiscoveryServiceTest, ServiceDoesNotStartWhenDisabled) {
  // Test disabled via enabled pref
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, false);
  CreateService();
  EXPECT_EQ(0, url_loader_factory_.NumPending());
}

TEST_F(WebDiscoveryServiceTest, DisablingCancelsPendingConfigRequests) {
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, true);
  CreateService();
  ASSERT_TRUE(url_loader_factory_.IsPending(GetQuorumHost() + "/config"));

  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, false);
  EXPECT_EQ(0, url_loader_factory_.NumPending());
}

TEST_F(WebDiscoveryServiceTest, ServiceTogglesOnPrefChange) {
  // Start disabled
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, false);
  CreateService();
  EXPECT_EQ(0, url_loader_factory_.NumPending());

  // Enable
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, true);
  ASSERT_NO_FATAL_FAILURE(ExpectCredentialJoin());

  // Disable again
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, false);
  EXPECT_EQ(0, url_loader_factory_.NumPending());

  // Enable again
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, true);
  ASSERT_NO_FATAL_FAILURE(ExpectCredentialJoin());
}

}  // namespace web_discovery
