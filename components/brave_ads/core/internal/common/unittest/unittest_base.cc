/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_build_channel_types.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_local_state_pref_registry.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_local_state_pref_value.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_registry.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_value.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/public/database/database.h"

namespace brave_ads {

namespace {

constexpr char kIfTimeStoodStill[] =
    "If time stood still, each moment would be stopped; frozen";

constexpr char kYouCantTravelBackInTime[] =
    "You Can't Travel Back in Time, Scientists Say! Unless, of course, you are "
    "travelling at 88 mph";

}  // namespace

UnitTestBase::UnitTestBase()
    : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
      scoped_default_locale_(
          brave_l10n::test::ScopedDefaultLocale(kDefaultLocale)) {
  CHECK(temp_dir_.CreateUniqueTempDir());

  set_ads_client_notifier_task_environment(&task_environment_);
}

UnitTestBase::~UnitTestBase() {
  CHECK(setup_called_)
      << "You have overridden SetUp but never called UnitTestBase::SetUp";

  CHECK(teardown_called_)
      << "You have overridden TearDown but never called UnitTestBase::TearDown";
}

void UnitTestBase::SetUp() {
  SetUp(/*is_integration_test=*/false);  // Default to unit test.
}

void UnitTestBase::TearDown() {
  teardown_called_ = true;

  ShutdownCommandLineSwitches();
}

void UnitTestBase::SetUp(const bool is_integration_test) {
  setup_called_ = true;

  is_integration_test_ = is_integration_test;

  InitializeCommandLineSwitches();

  RegisterProfilePrefs();

  RegisterLocalStatePrefs();

  MockAdsClientNotifier();

  MockAdsClient();

  is_integration_test_ ? SetUpIntegrationTest() : SetUpUnitTest();
}

AdsImpl& UnitTestBase::GetAds() const {
  CHECK(is_integration_test_) << "GetAds should only be called if SetUp is "
                                 "initialized for integration testing";

  CHECK(ads_);
  return *ads_;
}

bool UnitTestBase::CopyFileFromTestPathToTempPath(
    const std::string& from_path,
    const std::string& to_path) const {
  CHECK(setup_called_)
      << "CopyFileFromTestPathToTempPath should be called after SetUp";

  const base::FilePath from_test_path = TestDataPath().AppendASCII(from_path);
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
      << "CopyDirectoryFromTestPathToTempPath should be called after SetUp";

  const base::FilePath from_test_path = TestDataPath().AppendASCII(from_path);
  const base::FilePath to_temp_path = temp_dir_.GetPath().AppendASCII(to_path);

  return base::CopyDirectory(from_test_path, to_temp_path,
                             /*recursive=*/true);
}

bool UnitTestBase::CopyDirectoryFromTestPathToTempPath(
    const std::string& path) const {
  return CopyDirectoryFromTestPathToTempPath(path, path);
}

void UnitTestBase::FastForwardClockBy(const base::TimeDelta time_delta) {
  CHECK(!time_delta.is_zero()) << kIfTimeStoodStill;
  CHECK(time_delta.is_positive()) << kYouCantTravelBackInTime;

  task_environment_.FastForwardBy(time_delta);
}

void UnitTestBase::SuspendedFastForwardClockBy(
    const base::TimeDelta time_delta) {
  CHECK(!time_delta.is_zero()) << kIfTimeStoodStill;
  CHECK(time_delta.is_positive()) << kYouCantTravelBackInTime;

  task_environment_.SuspendedFastForwardBy(time_delta);
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
  CHECK(!time_delta.is_zero()) << kIfTimeStoodStill;
  CHECK(time_delta.is_positive()) << kYouCantTravelBackInTime;

  task_environment_.AdvanceClock(time_delta);
}

void UnitTestBase::AdvanceClockTo(const base::Time time) {
  return AdvanceClockBy(time - Now());
}

void UnitTestBase::AdvanceClockToLocalMidnight() {
  return AdvanceClockTo(Now().LocalMidnight() + base::Days(1));
}

void UnitTestBase::AdvanceClockToUTCMidnight() {
  return AdvanceClockTo(Now().UTCMidnight() + base::Days(1));
}

///////////////////////////////////////////////////////////////////////////////

void UnitTestBase::MockAdsClientNotifier() {
  MockAdsClientNotifierAddObserver(ads_client_mock_, *this);
}

void UnitTestBase::MockAdsClient() {
  // See to `common/unittest/unittest_mock_util.h`. `MockUrlRequest`,
  // `ShowScheduledCaptcha`, `RecordP2PAdEvents`, and `Log` are not mocked here;
  // they should be mocked as needed.

  MockIsNetworkConnectionAvailable(ads_client_mock_, true);

  MockIsBrowserActive(ads_client_mock_, true);
  MockIsBrowserInFullScreenMode(ads_client_mock_, false);

  MockCanShowNotificationAds(ads_client_mock_, true);
  MockCanShowNotificationAdsWhileBrowserIsBackgrounded(ads_client_mock_, false);
  MockShowNotificationAd(ads_client_mock_);
  MockCloseNotificationAd(ads_client_mock_);

  MockCacheAdEventForInstanceId(ads_client_mock_);
  MockGetCachedAdEvents(ads_client_mock_);
  MockResetAdEventCacheForInstanceId(ads_client_mock_);

  MockGetBrowsingHistory(ads_client_mock_, /*history=*/{});

  MockSave(ads_client_mock_);
  MockLoad(ads_client_mock_, temp_dir_);

  MockLoadComponentResource(ads_client_mock_, temp_dir_);

  MockLoadDataResource(ads_client_mock_);

  database_ = std::make_unique<Database>(
      temp_dir_.GetPath().AppendASCII(kDatabaseFilename));
  MockRunDBTransaction(ads_client_mock_, *database_);

  MockGetProfilePref(ads_client_mock_);
  MockSetProfilePref(ads_client_mock_, *this);
  MockClearProfilePref(ads_client_mock_);
  MockHasProfilePrefPath(ads_client_mock_);

  MockGetLocalStatePref(ads_client_mock_);
  MockSetLocalStatePref(ads_client_mock_, *this);
  MockClearLocalStatePref(ads_client_mock_);
  MockHasLocalStatePrefPath(ads_client_mock_);
}

void UnitTestBase::Mock() {
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  MockBuildChannel(BuildChannelType::kRelease);

  SetUpMocks();

  // Must be called after `SetUpMocks` because `SetupMocks` may call
  // `AppendCommandLineSwitches`.
  MockFlags();
}

void UnitTestBase::SetUpIntegrationTest() {
  CHECK(is_integration_test_)
      << "SetUpIntegrationTest should only be called if SetUp is initialized "
         "for integration testing";

  ads_ = std::make_unique<AdsImpl>(&ads_client_mock_);

  // Must be called after `AdsImpl` is instantiated but prior to `Initialize`.
  Mock();

  ads_->Initialize(test::WalletPtr(),
                   base::BindOnce(&UnitTestBase::SetUpIntegrationTestCallback,
                                  weak_factory_.GetWeakPtr()));
}

void UnitTestBase::SetUpIntegrationTestCallback(const bool success) {
  CHECK(success) << "Failed to initialize ads";

  // By default, integration tests are run while the browser is in the
  // foreground and active. If tests require the browser to be in the background
  // and inactive, you can call `NotifyBrowserDidEnterBackground` and
  // `NotifyBrowserDidResignActive`. Refer to `AdsClientNotifierForTesting` for
  // more information.
  NotifyBrowserDidEnterForeground();
  NotifyBrowserDidBecomeActive();

  NotifyDidInitializeAds();
}

void UnitTestBase::SetUpUnitTest() {
  CHECK(!is_integration_test_)
      << "SetUpUnitTest should only be called if SetUp is not initialized for "
         "integration testing";

  global_state_ = std::make_unique<GlobalState>(&ads_client_mock_);

  // Must be called after `GlobalState` is instantiated but prior to
  // `LoadState`.
  Mock();

  LoadState();
}

}  // namespace brave_ads
