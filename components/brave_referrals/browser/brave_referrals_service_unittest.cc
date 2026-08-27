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
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/constants/network_constants.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/data_element.h"
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

          if (request.url.path() == kBraveConversionPath) {
            conversion_request_count_++;
            conversion_request_body_ =
                std::string(request.request_body->elements()
                                ->at(0)
                                .As<network::DataElementBytes>()
                                .AsStringPiece());
          }

          url_loader_factory_.AddResponse(request.url.spec(), response_body);
          request_count_++;
        }));

    referral_initialized_callback_ =
        base::BindLambdaForTesting([&](const std::string& download_id) {
          init_callback_called_ = true;
          received_download_id_ = download_id;
        });
    BraveReferralsService::SetReferralInitializedCallbackForTesting(
        &referral_initialized_callback_);
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
  size_t conversion_request_count_ = 0;
  std::string conversion_request_body_;
  bool init_callback_called_ = false;
  std::string received_download_id_;
  BraveReferralsService::ReferralInitializedCallback
      referral_initialized_callback_;
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

#if BUILDFLAG(IS_ANDROID)

namespace {
constexpr char kTestGbraid[] = "GBRAIDTEST";
}  // namespace

TEST_F(BraveReferralsServiceTest, AndroidConversionNotSentWithoutGbraid) {
  CreateReferralsService();
  referrals_service_->Start();

  task_environment_.FastForwardBy(base::Minutes(1));

  EXPECT_EQ(conversion_request_count_, 0u);
}

// The run that captures the gbraid must not report: Start() reads the pref
// before the referrer fetch it kicks off can possibly have written it.
TEST_F(BraveReferralsServiceTest, AndroidConversionNotSentOnCaptureRun) {
  CreateReferralsService();
  referrals_service_->Start();

  // Stands in for the async referrer arriving later in the same run.
  pref_service_.SetString(kReferralAndroidGbraid, kTestGbraid);

  task_environment_.FastForwardBy(base::Minutes(1));

  EXPECT_EQ(conversion_request_count_, 0u);
  EXPECT_EQ(pref_service_.GetString(kReferralAndroidGbraid), kTestGbraid);
}

TEST_F(BraveReferralsServiceTest, AndroidConversionSentOnFollowingRun) {
  pref_service_.SetString(kReferralAndroidGbraid, kTestGbraid);

  CreateReferralsService();
  referrals_service_->Start();

  task_environment_.FastForwardBy(base::Minutes(1));

  EXPECT_EQ(conversion_request_count_, 1u);
  EXPECT_TRUE(pref_service_.GetString(kReferralAndroidGbraid).empty());

  base::DictValue body = base::test::ParseJsonDict(conversion_request_body_);
  EXPECT_EQ(*body.FindString("app_event_name"), "brave_second_open");
  EXPECT_EQ(*body.FindString("gbraid"), kTestGbraid);
  EXPECT_FALSE(body.FindString("app_version")->empty());
  EXPECT_FALSE(body.FindString("os_version")->empty());
  EXPECT_FALSE(body.FindString("sdk_version")->empty());
}

TEST_F(BraveReferralsServiceTest, AndroidConversionSentOnlyOnce) {
  pref_service_.SetString(kReferralAndroidGbraid, kTestGbraid);

  CreateReferralsService();
  referrals_service_->Start();
  task_environment_.FastForwardBy(base::Minutes(1));
  ASSERT_EQ(conversion_request_count_, 1u);

  // A subsequent run finds nothing left to report.
  referrals_service_.reset();
  CreateReferralsService();
  referrals_service_->Start();
  task_environment_.FastForwardBy(base::Minutes(1));

  EXPECT_EQ(conversion_request_count_, 1u);
}

TEST_F(BraveReferralsServiceTest, AndroidConversionDroppedWhenStatsDisabled) {
  pref_service_.SetString(kReferralAndroidGbraid, kTestGbraid);
  pref_service_.SetBoolean(kStatsReportingEnabled, false);

  CreateReferralsService();
  referrals_service_->Start();

  task_environment_.FastForwardBy(base::Minutes(1));

  EXPECT_EQ(conversion_request_count_, 0u);
  EXPECT_TRUE(pref_service_.GetString(kReferralAndroidGbraid).empty());
}

TEST_F(BraveReferralsServiceTest, AndroidConversionDroppedOnRequestFailure) {
  pref_service_.SetString(kReferralAndroidGbraid, kTestGbraid);
  url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        if (request.url.path() == kBraveConversionPath) {
          conversion_request_count_++;
        }
        url_loader_factory_.AddResponse(request.url.spec(), std::string(),
                                        net::HTTP_INTERNAL_SERVER_ERROR);
      }));

  CreateReferralsService();
  referrals_service_->Start();

  task_environment_.FastForwardBy(base::Minutes(1));

  EXPECT_EQ(conversion_request_count_, 1u);
  EXPECT_TRUE(pref_service_.GetString(kReferralAndroidGbraid).empty());
}

#endif  // BUILDFLAG(IS_ANDROID)

// The finalization checks timer interval is drawn from a geometric
// distribution, which can return 0 or a handful of seconds. Such an interval
// would make the repeating timer refire immediately (and, for 0, forever), so
// it has to be clamped to a non-zero minimum.
TEST(BraveReferralsServiceIntervalTest, FinalizationChecksIntervalIsClamped) {
  EXPECT_EQ(BraveReferralsService::GetFinalizationChecksInterval(0),
            base::Hours(1));
  EXPECT_EQ(BraveReferralsService::GetFinalizationChecksInterval(1),
            base::Hours(1));
  EXPECT_EQ(BraveReferralsService::GetFinalizationChecksInterval(
                base::Hours(1).InSeconds() - 1),
            base::Hours(1));
  EXPECT_EQ(BraveReferralsService::GetFinalizationChecksInterval(
                base::Days(1).InSeconds()),
            base::Days(1));
}

}  // namespace brave
