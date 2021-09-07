/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/unittest_base.h"

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

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
  CHECK(temp_dir_.CreateUniqueTempDir());

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

void UnitTestBase::TearDown() {
  // Code here will be called immediately after each test (right before the
  // destructor)

  teardown_called_ = true;
}

// Objects declared here can be used by all tests in the test case

bool UnitTestBase::CopyFileFromTestPathToTempDir(
    const std::string& source_filename,
    const std::string& dest_filename) const {
  CHECK(!setup_called_)
      << "|CopyFileFromTestPathToTempDir| should be called before "
         "|SetUpForTesting|";

  const base::FilePath from_path = GetTestPath().AppendASCII(source_filename);

  const base::FilePath to_path = temp_dir_.GetPath().AppendASCII(dest_filename);

  return base::CopyFile(from_path, to_path);
}

void UnitTestBase::SetUpForTesting(const bool integration_test) {
  setup_called_ = true;

  integration_test_ = integration_test;

  Initialize();
}

void UnitTestBase::InitializeAds() {
  CHECK(integration_test_)
      << "|InitializeAds| should only be called if "
         "|SetUpForTesting| was initialized for integration testing";

  ads_->Initialize([=](const bool success) {
    ASSERT_TRUE(success);

    ads_->OnWalletUpdated("c387c2d8-a26d-4451-83e4-5c0c6fd942be",
                          "5BEKM1Y7xcRSg/1q8in/+Lki2weFZQB+UMYZlRw8ql8=");
  });

  task_environment_.RunUntilIdle();
}

AdsImpl* UnitTestBase::GetAds() const {
  return ads_.get();
}

AdRewards* UnitTestBase::GetAdRewards() const {
  return ad_rewards_.get();
}

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
  SetEnvironment(mojom::Environment::kDevelopment);

  SetSysInfo(mojom::SysInfo());

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
  MockResetAdEvents(ads_client_mock_);

  MockGetBrowsingHistory(ads_client_mock_);

  MockLoad(ads_client_mock_, temp_dir_);
  MockLoadAdsResource(ads_client_mock_);
  MockLoadResourceForId(ads_client_mock_);
  MockSave(ads_client_mock_);

  MockPrefs(ads_client_mock_);

  const base::FilePath path =
      temp_dir_.GetPath().AppendASCII(kDatabaseFilename);
  database_ = std::make_unique<Database>(path);
  MockRunDBTransaction(ads_client_mock_, database_);

  if (integration_test_) {
    ads_ = std::make_unique<AdsImpl>(ads_client_mock_.get());
    return;
  }

  ads_client_helper_ =
      std::make_unique<AdsClientHelper>(ads_client_mock_.get());

  client_ = std::make_unique<Client>();
  client_->Initialize([](const bool success) { ASSERT_TRUE(success); });

  ad_notifications_ = std::make_unique<AdNotifications>();
  ad_notifications_->Initialize(
      [](const bool success) { ASSERT_TRUE(success); });

  ad_rewards_ = std::make_unique<AdRewards>();

  confirmations_state_ =
      std::make_unique<ConfirmationsState>(ad_rewards_.get());
  confirmations_state_->Initialize(
      [](const bool success) { ASSERT_TRUE(success); });

  database_initialize_ = std::make_unique<database::Initialize>();
  database_initialize_->CreateOrOpen(
      [](const bool success) { ASSERT_TRUE(success); });

  browser_manager_ = std::make_unique<BrowserManager>();

  tab_manager_ = std::make_unique<TabManager>();

  user_activity_ = std::make_unique<UserActivity>();

  // Fast forward until no tasks remain to ensure "EnsureSqliteInitialized"
  // tasks have fired before running tests
  task_environment_.FastForwardUntilNoTasksRemain();
}

}  // namespace ads
