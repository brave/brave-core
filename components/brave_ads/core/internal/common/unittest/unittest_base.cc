/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_local_state_pref_registry.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_local_state_pref_value.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_registry.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_value.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/public/database/database.h"

namespace brave_ads {

UnitTestBase::UnitTestBase()
    : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
      scoped_default_locale_(brave_l10n::test::ScopedDefaultLocale(  // IN-TEST
          kDefaultLocale)) {
  CHECK(temp_dir_.CreateUniqueTempDir());
}

UnitTestBase::~UnitTestBase() {
  CHECK(setup_called_)
      << "You have overridden SetUp but never called UnitTestBase::SetUp";

  CHECK(teardown_called_)
      << "You have overridden TearDown but never called UnitTestBase::TearDown";
}

void UnitTestBase::SetUp() {
  SetUpForTesting(/*is_integration_test=*/false);  // IN-TEST
}

void UnitTestBase::TearDown() {
  teardown_called_ = true;

  ShutdownCommandLineSwitches();
}

void UnitTestBase::SetUpForTesting(const bool is_integration_test) {
  setup_called_ = true;

  is_integration_test_ = is_integration_test;

  InitializeCommandLineSwitches();

  RegisterProfilePrefs();

  RegisterLocalStatePrefs();

  MockAdsClient();

  is_integration_test_ ? SetUpIntegrationTest() : SetUpUnitTest();
}

AdsImpl& UnitTestBase::GetAds() const {
  CHECK(is_integration_test_)
      << "|GetAds| should only be called if |SetUpForTesting| is initialized "
         "for integration testing";

  CHECK(ads_);

  return *ads_;
}

bool UnitTestBase::CopyFileFromTestPathToTempPath(
    const std::string& from_path,
    const std::string& to_path) const {
  CHECK(setup_called_)
      << "|CopyFileFromTestPathToTempPath| should be called after "
         "|SetUpForTesting|";

  const base::FilePath from_test_path = GetTestPath().AppendASCII(from_path);
  const base::FilePath to_temp_path = temp_dir_.GetPath().AppendASCII(to_path);

  return base::CopyFile(from_test_path, to_temp_path);
}

bool UnitTestBase::CopyFileFromTestPathToTempPath(
    const std::string& path) const {
  return CopyFileFromTestPathToTempPath(path, path);
}

bool UnitTestBase::CopyDirectoryFromTestPathToTempPath(
    const std::string& from_path,
    const std::string& to_path) const {
  CHECK(setup_called_)
      << "|CopyDirectoryFromTestPathToTempPath| should be called after "
         "|SetUpForTesting|";

  const base::FilePath from_test_path = GetTestPath().AppendASCII(from_path);
  const base::FilePath to_temp_path = temp_dir_.GetPath().AppendASCII(to_path);

  return base::CopyDirectory(from_test_path, to_temp_path,
                             /*recursive=*/true);
}

bool UnitTestBase::CopyDirectoryFromTestPathToTempPath(
    const std::string& path) const {
  return CopyDirectoryFromTestPathToTempPath(path, path);
}

void UnitTestBase::FastForwardClockBy(const base::TimeDelta time_delta) {
  CHECK(!time_delta.is_zero())
      << "If time stood still, each moment would be stopped; frozen";

  CHECK(time_delta.is_positive())
      << "You Can't Travel Back in Time, Scientists Say! Unless, of course, "
         "you are travelling at 88 mph";

  task_environment_.FastForwardBy(time_delta);
}

void UnitTestBase::FastForwardClockTo(const base::Time time) {
  FastForwardClockBy(time - Now());
}

void UnitTestBase::FastForwardClockToNextPendingTask() {
  CHECK(HasPendingTasks()) << "There are no pending tasks";
  task_environment_.FastForwardBy(NextPendingTaskDelay());
}

base::TimeDelta UnitTestBase::NextPendingTaskDelay() const {
  return task_environment_.NextMainThreadPendingTaskDelay();
}

size_t UnitTestBase::GetPendingTaskCount() const {
  return task_environment_.GetPendingMainThreadTaskCount();
}

bool UnitTestBase::HasPendingTasks() const {
  return GetPendingTaskCount() > 0;
}

void UnitTestBase::AdvanceClockBy(const base::TimeDelta time_delta) {
  CHECK(!time_delta.is_zero())
      << "If time stood still, each moment would be stopped; frozen";

  CHECK(time_delta.is_positive())
      << "You Can't Travel Back in Time, Scientists Say! Unless, of course, "
         "you are travelling at 88 mph";

  task_environment_.AdvanceClock(time_delta);
}

void UnitTestBase::AdvanceClockTo(const base::Time time) {
  return AdvanceClockBy(time - Now());
}

void UnitTestBase::AdvanceClockToMidnight(const bool is_local) {
  const base::Time midnight_rounded_down_to_nearest_day =
      is_local ? Now().LocalMidnight() : Now().UTCMidnight();
  return AdvanceClockTo(midnight_rounded_down_to_nearest_day + base::Days(1));
}

///////////////////////////////////////////////////////////////////////////////

void UnitTestBase::MockAdsClientAddObserver() {
  ON_CALL(ads_client_mock_, AddObserver)
      .WillByDefault(
          ::testing::Invoke([=](AdsClientNotifierObserver* observer) {
            CHECK(observer);
            AddObserver(observer);
          }));
}

void UnitTestBase::MockAdsClient() {
  MockAdsClientAddObserver();

  MockShowNotificationAd(ads_client_mock_);
  MockCloseNotificationAd(ads_client_mock_);

  MockCacheAdEventForInstanceId(ads_client_mock_);
  MockGetCachedAdEvents(ads_client_mock_);
  MockResetAdEventCacheForInstanceId(ads_client_mock_);

  MockSave(ads_client_mock_);
  MockLoad(ads_client_mock_, temp_dir_);
  MockLoadFileResource(ads_client_mock_, temp_dir_);
  MockLoadDataResource(ads_client_mock_);

  database_ = std::make_unique<Database>(
      temp_dir_.GetPath().AppendASCII(kDatabaseFilename));
  MockRunDBTransaction(ads_client_mock_, *database_);

  MockGetProfilePref(ads_client_mock_);
  MockSetProfilePref();
  MockClearProfilePref(ads_client_mock_);
  MockHasProfilePrefPath(ads_client_mock_);

  MockGetLocalStatePref(ads_client_mock_);
  MockSetLocalStatePref();
  MockClearLocalStatePref(ads_client_mock_);
  MockHasLocalStatePrefPath(ads_client_mock_);

  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  MockIsNetworkConnectionAvailable(ads_client_mock_, true);

  MockIsBrowserActive(ads_client_mock_, true);

  MockIsBrowserInFullScreenMode(ads_client_mock_, false);

  MockCanShowNotificationAds(ads_client_mock_, true);
  MockCanShowNotificationAdsWhileBrowserIsBackgrounded(ads_client_mock_, false);

  MockGetBrowsingHistory(ads_client_mock_, /*history=*/{});
}

void UnitTestBase::MockSetProfilePref() {
  ON_CALL(ads_client_mock_, SetProfilePref)
      .WillByDefault(
          ::testing::Invoke([=](const std::string& path, base::Value value) {
            SetProfilePrefValue(path, std::move(value));
            NotifyPrefDidChange(path);
          }));
}

void UnitTestBase::MockSetLocalStatePref() {
  ON_CALL(ads_client_mock_, SetLocalStatePref)
      .WillByDefault(
          ::testing::Invoke([=](const std::string& path, base::Value value) {
            SetLocalStatePrefValue(path, std::move(value));
            NotifyPrefDidChange(path);
          }));
}

void UnitTestBase::SetUpTest() {
  MockBuildChannel(BuildChannelType::kRelease);

  SetUpMocks();

  MockFlags();
}

void UnitTestBase::SetUpIntegrationTest() {
  CHECK(is_integration_test_)
      << "|SetUpIntegrationTest| should only be called if |SetUpForTesting| is "
         "initialized for integration testing";

  ads_ = std::make_unique<AdsImpl>(&ads_client_mock_);

  SetUpTest();

  ads_->Initialize(GetWalletPtrForTesting(),  // IN-TEST
                   base::BindOnce(&UnitTestBase::SetUpIntegrationTestCallback,
                                  weak_factory_.GetWeakPtr()));
}

void UnitTestBase::SetUpIntegrationTestCallback(const bool success) {
  ASSERT_TRUE(success) << "Failed to initialize ads";

  NotifyDidInitializeAds();

  task_environment_.RunUntilIdle();
}

void UnitTestBase::SetUpUnitTest() {
  CHECK(!is_integration_test_)
      << "|SetUpUnitTest| should only be called if |SetUpForTesting| is not "
         "initialized for integration testing";

  global_state_ = std::make_unique<GlobalState>(&ads_client_mock_);

  SetUpTest();

  global_state_->GetDatabaseManager().CreateOrOpen(
      base::BindOnce([](const bool success) {
        ASSERT_TRUE(success) << "Failed to create or open database";
      }));

  global_state_->GetClientStateManager().Load(
      base::BindOnce([](const bool success) {
        ASSERT_TRUE(success) << "Failed to load client state";
      }));

  global_state_->GetConfirmationStateManager().Load(
      GetWalletForTesting(),  // IN-TEST
      base::BindOnce([](const bool success) {
        ASSERT_TRUE(success) << "Failed to load confirmation state";
      }));

  task_environment_.FastForwardUntilNoTasksRemain();
}

}  // namespace brave_ads
