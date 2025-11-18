// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_referrals/browser/brave_referrals_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/bind.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

namespace {

constexpr char kTestPromoCode[] = "TEST123";
constexpr char kTestDownloadId[] = "test-download-id";

}  // namespace

class BraveReferralsServiceTest : public testing::Test {
 public:
  BraveReferralsServiceTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    // Set up environment for testing
    RegisterPrefsForBraveReferralsService(pref_service_.registry());
    pref_service_.registry()->RegisterBooleanPref(kStatsReportingEnabled, true);

    // Create a test promo code file
    auto promo_code_file_path = temp_dir_.GetPath().AppendASCII("promoCode");
    ASSERT_TRUE(base::WriteFile(promo_code_file_path, kTestPromoCode));
    BraveReferralsService::SetPromoFilePathForTesting(promo_code_file_path);

    // Set up URL loader factory interceptor
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          std::string response_body;
          if (request.url.path().starts_with("/promo/initialize")) {
            // Respond with a successful init response
            response_body =
                "{\"download_id\":\"" + std::string(kTestDownloadId) + "\"}";
          } else if (request.url.path() == "/promo/activity") {
            // Respond with a successful finalization check response
            response_body = "{\"finalized\":true}";
          }

          url_loader_factory_.AddResponse(request.url.spec(), response_body);
          request_count_++;
        }));

    BraveReferralsService::SetReferralInitializedCallbackForTesting(
        new BraveReferralsService::ReferralInitializedCallback(
            base::BindLambdaForTesting([&](const std::string& download_id) {
              init_callback_called_ = true;
              received_download_id_ = download_id;
            })));
  }

  void TearDown() override {
    referrals_service_.reset();
    BraveReferralsService::SetPromoFilePathForTesting(base::FilePath());
    BraveReferralsService::SetReferralInitializedCallbackForTesting(nullptr);
  }

  class TestBraveReferralsServiceDelegate
      : public BraveReferralsService::Delegate {
   public:
    TestBraveReferralsServiceDelegate() = default;
    ~TestBraveReferralsServiceDelegate() override = default;

    void OnInitialized() override {}

    base::FilePath GetUserDataDirectory() override { return user_data_dir_; }

    network::mojom::URLLoaderFactory* GetURLLoaderFactory() override {
      return loader_factory;
    }

#if !BUILDFLAG(IS_ANDROID)
    base::OnceCallback<base::Time()> GetFirstRunSentinelCreationTimeCallback()
        override {
      return base::BindLambdaForTesting([this]() { return first_run_time_; });
    }
#endif

    base::FilePath user_data_dir_;
    base::Time first_run_time_;
    raw_ptr<network::mojom::URLLoaderFactory> loader_factory = nullptr;
  };

  void CreateReferralsService() {
    auto test_delegate = std::make_unique<TestBraveReferralsServiceDelegate>();
    test_delegate->user_data_dir_ = temp_dir_.GetPath();
    test_delegate->loader_factory = shared_url_loader_factory_.get();
    test_delegate->first_run_time_ = first_run_time_;

    referrals_service_ = std::make_unique<BraveReferralsService>(
        &pref_service_, "test", "test-api-key");
    referrals_service_->set_delegate(std::move(test_delegate));
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  base::Time first_run_time_ = base::Time::Now();
  network::TestURLLoaderFactory url_loader_factory_;
  TestingPrefServiceSimple pref_service_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<BraveReferralsService> referrals_service_;
  size_t request_count_ = 0;
  bool init_callback_called_ = false;
  std::string received_download_id_;
};

TEST_F(BraveReferralsServiceTest, InitAndActivity) {
  CreateReferralsService();
  referrals_service_->Start();

  task_environment_.FastForwardBy(base::Minutes(1));

  EXPECT_TRUE(init_callback_called_);
  EXPECT_EQ(received_download_id_, kTestDownloadId);

  EXPECT_EQ(pref_service_.GetString(kReferralDownloadID), kTestDownloadId);
  EXPECT_TRUE(pref_service_.GetBoolean(kReferralInitialization));
  EXPECT_EQ(request_count_, 1u);

  task_environment_.FastForwardBy(base::Days(35));

  EXPECT_FALSE(pref_service_.HasPrefPath(kReferralAttemptTimestamp));
  EXPECT_FALSE(pref_service_.HasPrefPath(kReferralAttemptCount));
  EXPECT_GE(request_count_, 2u);
}

TEST_F(BraveReferralsServiceTest, StatsDisabledAtInit) {
  pref_service_.SetBoolean(kStatsReportingEnabled, false);

  CreateReferralsService();
  referrals_service_->Start();

  task_environment_.FastForwardBy(base::Minutes(1));

  EXPECT_TRUE(init_callback_called_);
  EXPECT_EQ(received_download_id_, std::string());

  task_environment_.FastForwardBy(base::Days(35));

  EXPECT_EQ(request_count_, 0u);
}

TEST_F(BraveReferralsServiceTest, StatsDisabledAfterInit) {
  CreateReferralsService();
  referrals_service_->Start();

  task_environment_.FastForwardBy(base::Minutes(1));

  EXPECT_TRUE(init_callback_called_);
  EXPECT_EQ(received_download_id_, kTestDownloadId);

  EXPECT_EQ(pref_service_.GetString(kReferralDownloadID), kTestDownloadId);
  EXPECT_TRUE(pref_service_.GetBoolean(kReferralInitialization));
  EXPECT_EQ(request_count_, 1u);

  pref_service_.SetBoolean(kStatsReportingEnabled, false);

  task_environment_.FastForwardBy(base::Days(35));

  EXPECT_GE(request_count_, 1u);
}

}  // namespace brave
