/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/unittest_base.h"

#include "base/files/file_path.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"

using ::testing::NiceMock;

namespace ads {

namespace {
const char kDatabaseFilename[] = "database.sqlite";
}  // namespace

UnitTestBase::UnitTestBase()
    : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
      ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
      locale_helper_mock_(
          std::make_unique<NiceMock<brave_l10n::LocaleHelperMock>>()),
      platform_helper_mock_(std::make_unique<NiceMock<PlatformHelperMock>>()) {
  // You can do set-up work for each test here
  brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
      locale_helper_mock_.get());

  PlatformHelper::GetInstance()->set_for_testing(platform_helper_mock_.get());
}

UnitTestBase::~UnitTestBase() {
  // You can do clean-up work that doesn't throw exceptions here

  CHECK(setup_called_)
      << "You have overridden SetUp but never called UnitTestBase::SetUp";

  CHECK(teardown_called_)
      << "You have overridden TearDown but never called UnitTestBase::TearDown";
}

void UnitTestBase::SetUp() {
  // Code here will be called immediately after the constructor (right before
  // each test)

  SetUpForTesting(/* integration_test */ false);
}

void UnitTestBase::SetUpForTesting(const bool integration_test) {
  setup_called_ = true;

  integration_test_ = integration_test;

  Initialize();
}

void UnitTestBase::TearDown() {
  // Code here will be called immediately after each test (right before the
  // destructor)

  teardown_called_ = true;
}

// Objects declared here can be used by all tests in the test case

void UnitTestBase::FastForwardClockBy(const base::TimeDelta& time_delta) {
  task_environment_.FastForwardBy(time_delta);
}

void UnitTestBase::FastForwardClockTo(const base::Time& time) {
  const base::TimeDelta time_delta = time - base::Time::Now();

  FastForwardClockBy(time_delta);
}

void UnitTestBase::AdvanceClockToMidnightUTC() {
  const base::TimeDelta time_delta = base::Time::Now().LocalMidnight() +
                                     base::TimeDelta::FromHours(24) -
                                     base::Time::Now();

  return AdvanceClock(time_delta);
}

void UnitTestBase::AdvanceClock(const base::Time& time) {
  const base::TimeDelta time_delta = time - base::Time::Now();

  return AdvanceClock(time_delta);
}

void UnitTestBase::AdvanceClock(const base::TimeDelta& time_delta) {
  task_environment_.AdvanceClock(time_delta);
}

base::TimeDelta UnitTestBase::NextPendingTaskDelay() const {
  return task_environment_.NextMainThreadPendingTaskDelay();
}

size_t UnitTestBase::GetPendingTaskCount() const {
  return task_environment_.GetPendingMainThreadTaskCount();
}

///////////////////////////////////////////////////////////////////////////////

void UnitTestBase::Initialize() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

  SetEnvironment(Environment::DEVELOPMENT);

  SetSysInfo(SysInfo());

  SetBuildChannel(false, "test");

  MockLocaleHelper(locale_helper_mock_, "en-US");

  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  MockIsNetworkConnectionAvailable(ads_client_mock_, true);

  MockIsForeground(ads_client_mock_, true);

  MockIsFullScreen(ads_client_mock_, false);

  MockShouldShowNotifications(ads_client_mock_, true);
  MockShowNotification(ads_client_mock_);
  MockCloseNotification(ads_client_mock_);

  MockRecordAdEvent(ads_client_mock_);
  MockGetAdEvents(ads_client_mock_);

  MockLoad(ads_client_mock_);
  MockLoadAdsResource(ads_client_mock_);
  MockLoadResourceForId(ads_client_mock_);
  MockSave(ads_client_mock_);

  MockPrefs(ads_client_mock_);

  const base::FilePath path = temp_dir_.GetPath();
  database_ = std::make_unique<Database>(path.AppendASCII(kDatabaseFilename));
  MockRunDBTransaction(ads_client_mock_, database_);

  if (integration_test_) {
    ads_ = std::make_unique<AdsImpl>(ads_client_mock_.get());

    ads_->OnWalletUpdated("c387c2d8-a26d-4451-83e4-5c0c6fd942be",
                          "5BEKM1Y7xcRSg/1q8in/+Lki2weFZQB+UMYZlRw8ql8=");

    return;
  }

  ads_client_helper_ =
      std::make_unique<AdsClientHelper>(ads_client_mock_.get());

  client_ = std::make_unique<Client>();

  ad_notifications_ = std::make_unique<AdNotifications>();
  ad_notifications_->Initialize(
      [](const Result result) { ASSERT_EQ(Result::SUCCESS, result); });

  ad_rewards_ = std::make_unique<AdRewards>();

  confirmations_state_ =
      std::make_unique<ConfirmationsState>(ad_rewards_.get());
  confirmations_state_->Initialize(
      [](const Result result) { ASSERT_EQ(Result::SUCCESS, result); });

  database_initialize_ = std::make_unique<database::Initialize>();
  database_initialize_->CreateOrOpen(
      [](const Result result) { ASSERT_EQ(Result::SUCCESS, result); });

  browser_manager_ = std::make_unique<BrowserManager>();

  tab_manager_ = std::make_unique<TabManager>();

  user_activity_ = std::make_unique<UserActivity>();

  // Fast forward until no tasks remain to ensure "EnsureSqliteInitialized"
  // tasks have fired before running tests
  task_environment_.FastForwardUntilNoTasksRemain();
}

void UnitTestBase::InitializeAds() {
  CHECK(integration_test_)
      << "|InitializeAds| should only be called if "
         "|SetUpForTesting| was initialized for integration testing";

  ads_->Initialize(
      [](const Result result) { ASSERT_EQ(Result::SUCCESS, result); });

  task_environment_.RunUntilIdle();
}

}  // namespace ads
