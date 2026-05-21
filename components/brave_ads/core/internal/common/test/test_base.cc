/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

#include <memory>
#include <string_view>

#include "base/barrier_closure.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_ads/core/internal/account/tokens/test/fake_token_generator.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_state_manager.h"
#include "brave/components/brave_ads/core/internal/account/wallet/test/wallet_test_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_notifier_waiter.h"
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_types.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/command_line_switch_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/mock_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/test_environment_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/local_state_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/pref_registry_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_environment_util.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/public/ads.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads::test {

namespace {

constexpr std::string_view kIfTimeStoodStill =
    "If time stood still, each moment would be stopped; frozen";

constexpr std::string_view kYouCantTravelBackInTime =
    "You Can't Travel Back in Time, Scientists Say! Unless, of course, you are "
    "travelling at 88 mph";

}  // namespace

TestBase::TestBase()
    : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
      tab_helper_(ads_client_notifier_) {
  SimulateProfile();
}

TestBase::TestBase(bool is_integration_test) : TestBase() {
  is_integration_test_ = is_integration_test;
}

TestBase::~TestBase() {
  CHECK(setup_called_)
      << "You have overridden SetUp but never called TestBase::SetUp";

  CHECK(teardown_called_)
      << "You have overridden TearDown but never called TestBase::TearDown";
}

void TestBase::SetUp() {
  SetUp(is_integration_test_);
}

void TestBase::TearDown() {
  teardown_called_ = true;

  ResetCommandLineSwitches();
}

void TestBase::SetUp(bool is_integration_test) {
  setup_called_ = true;

  is_integration_test_ = is_integration_test;

  SimulateCommandLineSwitches();

  RegisterProfilePrefs();
  RegisterLocalStatePrefs();

  MockAdsClientNotifier();
  MockAdsClient();

  if (is_integration_test) {
    return SetUpIntegrationTest();
  }

  SetUpUnitTest();
}

Ads& TestBase::GetAds() const {
  CHECK(is_integration_test_) << "GetAds should only be called if SetUp is "
                                 "initialized for integration testing";
  CHECK(ads_) << "Ads instance is not initialized";

  return *ads_;
}

bool TestBase::CopyFileFromTestDataPathToProfilePath(
    const std::string& from_path,
    const std::string& to_path) const {
  CHECK(setup_called_)
      << "CopyFileFromTestDataPathToProfilePath should be called after SetUp";

  const base::FilePath from_test_data_path = DataPath().AppendASCII(from_path);
  const base::FilePath to_profile_path = ProfilePath().AppendASCII(to_path);

  return base::CopyFile(from_test_data_path, to_profile_path);
}

bool TestBase::CopyFileFromTestDataPathToProfilePath(
    const std::string& path) const {
  return CopyFileFromTestDataPathToProfilePath(/*from_path=*/path,
                                               /*to_path=*/path);
}

bool TestBase::CopyDirectoryFromTestDataPathToProfilePath(
    const std::string& from_path,
    const std::string& to_path,
    bool recursive) const {
  CHECK(setup_called_) << "CopyDirectoryFromTestDataPathToProfilePath should "
                          "be called after SetUp";

  const base::FilePath from_test_data_path = DataPath().AppendASCII(from_path);
  const base::FilePath to_profile_path = ProfilePath().AppendASCII(to_path);

  return base::CopyDirectory(from_test_data_path, to_profile_path, recursive);
}

bool TestBase::CopyDirectoryFromTestDataPathToProfilePath(
    const std::string& path,
    bool recursive) const {
  return CopyDirectoryFromTestDataPathToProfilePath(/*from_path=*/path,
                                                    /*to_path=*/path,
                                                    recursive);
}

void TestBase::FastForwardClockBy(base::TimeDelta time_delta) {
  CHECK(!time_delta.is_zero()) << kIfTimeStoodStill;
  CHECK(time_delta.is_positive()) << kYouCantTravelBackInTime;

  task_environment_.FastForwardBy(time_delta);
}

void TestBase::SuspendedFastForwardClockBy(base::TimeDelta time_delta) {
  CHECK(!time_delta.is_zero()) << kIfTimeStoodStill;
  CHECK(time_delta.is_positive()) << kYouCantTravelBackInTime;

  task_environment_.SuspendedFastForwardBy(time_delta);
}

void TestBase::FastForwardClockTo(base::Time time) {
  FastForwardClockBy(time - Now());
}

void TestBase::FastForwardClockToNextPendingTask() {
  CHECK(HasPendingTasks()) << "There are no pending tasks";

  task_environment_.FastForwardBy(NextPendingTaskDelay());
}

base::TimeDelta TestBase::NextPendingTaskDelay() {
  FlushImmediateTasks();

  return task_environment_.NextMainThreadPendingTaskDelay();
}

size_t TestBase::GetPendingTaskCount() {
  FlushImmediateTasks();

  return task_environment_.GetPendingMainThreadTaskCount();
}

bool TestBase::HasPendingTasks() {
  return GetPendingTaskCount() > 0;
}

void TestBase::AdvanceClockBy(base::TimeDelta time_delta) {
  CHECK(!time_delta.is_zero()) << kIfTimeStoodStill;
  CHECK(time_delta.is_positive()) << kYouCantTravelBackInTime;

  task_environment_.AdvanceClock(time_delta);
}

void TestBase::AdvanceClockTo(base::Time time) {
  return AdvanceClockBy(time - Now());
}

void TestBase::AdvanceClockToLocalMidnight() {
  return AdvanceClockTo(Now().LocalMidnight() + base::Days(1));
}

void TestBase::AdvanceClockToUTCMidnight() {
  return AdvanceClockTo(Now().UTCMidnight() + base::Days(1));
}

void TestBase::SimulateProfile() {
  CHECK(profile_dir_.CreateUniqueTempDir());

  std::cout << "SIMULATED PROFILE PATH: " << ProfilePath() << std::endl;
}

base::FilePath TestBase::DatabasePath() const {
  return ProfilePath().AppendASCII(kDatabaseFilename);
}

void TestBase::MockAdsClientNotifier() {
  MockAdsClientNotifierAddObserver(ads_client_mock_, ads_client_notifier_);
}

void TestBase::MockAdsClient() {
  // `MockUrlResponses`, `ShowScheduledCaptcha`, and `Log` are not mocked here;
  // they should be mocked as needed via `mock_test_util.h`.

  MockNotifyPendingObservers(ads_client_mock_, ads_client_notifier_);

  MockIsNetworkConnectionAvailable(ads_client_mock_, true);

  MockIsBrowserActive(ads_client_mock_, true);
  MockIsBrowserInFullScreenMode(ads_client_mock_, false);

  MockCanShowNotificationAds(ads_client_mock_, true);
  MockCanShowNotificationAdsWhileBrowserIsBackgrounded(ads_client_mock_, false);
  MockShowNotificationAd(ads_client_mock_);
  MockCloseNotificationAd(ads_client_mock_);

  MockGetSiteHistory(ads_client_mock_, /*site_history=*/{});

  MockSave(ads_client_mock_);
  MockRemove(ads_client_mock_);
  MockLoad(ads_client_mock_, ProfilePath());

  MockLoadResourceComponent(ads_client_mock_, ProfilePath());

  MockFindProfilePref(ads_client_mock_);
  MockGetProfilePref(ads_client_mock_);
  MockSetProfilePref(ads_client_mock_, ads_client_notifier_);
  MockClearProfilePref(ads_client_mock_);
  MockHasProfilePrefPath(ads_client_mock_);

  MockFindLocalStatePref(ads_client_mock_);
  MockGetLocalStatePref(ads_client_mock_);
  MockSetLocalStatePref(ads_client_mock_, ads_client_notifier_);
  MockClearLocalStatePref(ads_client_mock_);
  MockHasLocalStatePrefPath(ads_client_mock_);
}

void TestBase::SetUpEnvironment() {
  CHECK(GlobalState::HasInstance())
      << "Must be called after GlobalState is instantiated";

  fake_operating_system_.SetType(OperatingSystemType::kWindows);

  SetUpBuildChannel(BuildChannelType::kRelease);

  SetUpContentSettings();

  SetUpMocks();

  // Must be called after `SetUpMocks` because `SetupMocks` may call
  // `AppendCommandLineSwitches`.
  SetUpCommandLineSwitches();
}

void TestBase::SetUpDefaultAdsServiceState(
    base::OnceClosure initialized_callback) const {
  CHECK(!is_integration_test_)
      << "SetUpDefaultAdsServiceState should only be called if SetUp is "
         "initialized for unit testing";
  CHECK(GlobalState::HasInstance())
      << "Must be called after GlobalState is instantiated";

  // Both the database+token path and the client state path are async; the
  // barrier closure fires `initialized_callback` once both have completed.
  auto barrier_closure =
      base::BarrierClosure(/*num_closures=*/2, std::move(initialized_callback));

  GlobalState::GetInstance()->GetDatabaseManager().CreateOrOpen(base::BindOnce(
      [](const base::RepeatingClosure& barrier_closure, bool success) {
        ASSERT_TRUE(success) << "Failed to create or open database";

        GlobalState::GetInstance()->GetTokenStateManager().LoadState(
            base::BindOnce(
                [](const base::RepeatingClosure& barrier_closure,
                   bool success) {
                  ASSERT_TRUE(success) << "Failed to load token state";
                  barrier_closure.Run();
                },
                barrier_closure));
      },
      barrier_closure));

  // TODO(https://github.com/brave/brave-browser/issues/39795): Transition away
  // from using JSON state to a more efficient data approach.
  GlobalState::GetInstance()->GetClientStateManager().LoadState(base::BindOnce(
      [](const base::RepeatingClosure& barrier_closure, bool success) {
        ASSERT_TRUE(success) << "Failed to load client state";
        barrier_closure.Run();
      },
      barrier_closure));
}

void TestBase::SetUpIntegrationTest() {
  CHECK(is_integration_test_)
      << "SetUpIntegrationTest should only be called if SetUp is initialized "
         "for integration testing";

  ads_ = Ads::CreateInstance(ads_client_mock_, DatabasePath());
  CHECK(ads_) << "Failed to create ads instance";

  // Must be called after `Ads` is instantiated but prior to `Initialize`.
  SetUpEnvironment();

  ads_->Initialize(MojomWallet(),
                   base::BindOnce(&TestBase::SetUpIntegrationTestCallback,
                                  weak_factory_.GetWeakPtr()));

  AdsClientNotifierWaiter(/*ads_client_notifier=*/&ads_client_notifier_)
      .WaitForOnNotifyDidInitializeAds();

  // Flush 0-delay task replies posted during initialization.
  FlushImmediateTasks();
}

void TestBase::SetUpIntegrationTestCallback(bool success) {
  CHECK(success) << "Failed to initialize ads";

  // By default, integration tests are run while the browser is in the
  // foreground and active. If tests require the browser to be in the background
  // and inactive, you can call `NotifyBrowserDidEnterBackground` and
  // `NotifyBrowserDidResignActive`.
  ads_client_notifier_.NotifyBrowserDidEnterForeground();
  ads_client_notifier_.NotifyBrowserDidBecomeActive();

  ads_client_notifier_.NotifyDidInitializeAds();
}

void TestBase::SetUpUnitTest() {
  CHECK(!is_integration_test_) << "SetUpUnitTest should only be called if "
                                  "SetUp is initialized for unit testing";

  global_state_ = std::make_unique<GlobalState>(
      ads_client_mock_, DatabasePath(), std::make_unique<FakeTokenGenerator>());

  // Must be called after `GlobalState` is instantiated because it dispatches
  // notifications through it, and before `SetUpDefaultAdsServiceState` because
  // database creation and state loading depend on mocks, prefs, and feature
  // flags configured here.
  SetUpEnvironment();

  base::RunLoop run_loop;
  SetUpDefaultAdsServiceState(run_loop.QuitClosure());

  ads_client_notifier_.NotifyPendingObservers();

  // Process background thread replies without advancing the mock clock, so
  // that tests requiring a specific `NextPendingTaskDelay` are unaffected.
  run_loop.Run();
}

void TestBase::FlushImmediateTasks() {
  // `base::SequenceBound` always posts a completion reply to the calling
  // sequence, even for fire-and-forget calls with `base::DoNothing`. Those
  // replies arrive with a 0ms delay and must be flushed so that callers see
  // only genuinely scheduled tasks.
  if (task_environment_.NextMainThreadPendingTaskDelay().is_zero()) {
    task_environment_.FastForwardBy(base::TimeDelta());
  }
}

}  // namespace brave_ads::test
