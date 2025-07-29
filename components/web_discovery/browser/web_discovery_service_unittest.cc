/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/web_discovery_service.h"

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "base/memory/weak_ptr.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_search/browser/backup_results_service.h"
#include "brave/components/constants/pref_names.h"
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
                            BackupResultsCallback callback) override {}

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
    profile_prefs_.registry()->RegisterBooleanPref(
        kWebDiscoveryDisabledByPolicy, false);

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

 protected:
  void CreateService() {
    service_ = std::make_unique<WebDiscoveryService>(
        &local_state_, &profile_prefs_, temp_dir_.GetPath(),
        shared_url_loader_factory_, &backup_results_service_);
  }

  // Check if service is active (has both components)
  bool IsActive() const {
    return service_->server_config_loader_ != nullptr &&
           service_->credential_manager_ != nullptr;
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
  profile_prefs_.SetBoolean(kWebDiscoveryDisabledByPolicy, false);

  CreateService();

  EXPECT_TRUE(IsActive());
}

TEST_F(WebDiscoveryServiceTest, ServiceDoesNotStartWhenDisabled) {
  // Test disabled via enabled pref
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, false);
  profile_prefs_.SetBoolean(kWebDiscoveryDisabledByPolicy, false);
  CreateService();
  EXPECT_FALSE(IsActive());

  // Test disabled via policy pref
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, true);
  profile_prefs_.SetBoolean(kWebDiscoveryDisabledByPolicy, true);
  CreateService();
  EXPECT_FALSE(IsActive());

  // Test both disabled
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, false);
  profile_prefs_.SetBoolean(kWebDiscoveryDisabledByPolicy, true);
  CreateService();
  EXPECT_FALSE(IsActive());
}

TEST_F(WebDiscoveryServiceTest, ServiceTogglesOnPrefChange) {
  // Start disabled
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, false);
  CreateService();
  EXPECT_FALSE(IsActive());

  // Enable
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, true);
  EXPECT_TRUE(IsActive());

  // Disable again
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, false);
  EXPECT_FALSE(IsActive());

  // Enable again
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, true);
  EXPECT_TRUE(IsActive());
}

TEST_F(WebDiscoveryServiceTest, ServiceDoesNotToggleWhenDisabledByPolicy) {
  // Start with policy disabled
  profile_prefs_.SetBoolean(kWebDiscoveryDisabledByPolicy, true);
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, false);
  CreateService();
  EXPECT_FALSE(IsActive());

  // Try to enable via user pref - should remain inactive due to policy
  profile_prefs_.SetBoolean(kWebDiscoveryEnabled, true);
  EXPECT_FALSE(IsActive());
}

}  // namespace web_discovery
