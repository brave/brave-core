/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/unittest/unittest_base.h"

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/values.h"
#include "bat/ads/database.h"
#include "bat/ads/internal/account/wallet/wallet_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_command_line_switch_util.h"
#include "bat/ads/internal/common/unittest/unittest_constants.h"
#include "bat/ads/internal/common/unittest/unittest_file_util.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"

using ::testing::NiceMock;

namespace ads {

UnitTestBase::UnitTestBase()
    : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
      ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
      platform_helper_mock_(std::make_unique<NiceMock<PlatformHelperMock>>()),
      scoped_default_locale_(
          std::make_unique<brave_l10n::test::ScopedDefaultLocale>(  // IN-TEST
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
  SetUpForTesting(/*is_integration_test*/ false);  // IN-TEST
}

void UnitTestBase::TearDown() {
  teardown_called_ = true;

  CleanupCommandLineSwitches();
}

void UnitTestBase::SetUpForTesting(const bool is_integration_test) {
  setup_called_ = true;

  is_integration_test_ = is_integration_test;

  Initialize();
}

AdsImpl* UnitTestBase::GetAds() const {
  CHECK(is_integration_test_)
      << "|GetAds| should only be called if |SetUpForTesting| is initialized "
         "for integration testing";

  return ads_.get();
}

bool UnitTestBase::CopyFileFromTestPathToTempPath(
    const std::string& from_path,
    const std::string& to_path) const {
  CHECK(setup_called_)
      << "|CopyFileFromTestPathToTempPath| should be called after "
         "|SetUpForTesting|";

  const base::FilePath from_test_path = GetTestPath().AppendASCII(from_path);
  const base::FilePath to_temp_path = temp_dir_.GetPath().AppendASCII(to_path);

  const bool success = base::CopyFile(from_test_path, to_temp_path);
  CHECK(success) << "Failed to copy file from " << from_test_path << " to "
                 << to_temp_path;
  return success;
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

  const bool success = base::CopyDirectory(from_test_path, to_temp_path,
                                           /*recursive*/ true);
  CHECK(success) << "Failed to copy directory from " << from_test_path << " to "
                 << to_temp_path;
  return success;
}

bool UnitTestBase::CopyDirectoryFromTestPathToTempPath(
    const std::string& path) const {
  return CopyDirectoryFromTestPathToTempPath(path, path);
}

void UnitTestBase::FastForwardClockBy(const base::TimeDelta time_delta) {
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

void UnitTestBase::Initialize() {
  InitializeCommandLineSwitches();

  SetDefaultMocks();

  SetDefaultPrefs();

  if (!is_integration_test_) {
    ads_client_helper_ =
        std::make_unique<AdsClientHelper>(ads_client_mock_.get());
  }

  SetUpMocks();

  if (is_integration_test_) {
    SetUpIntegrationTest();
    return;
  }

  browser_manager_ = std::make_unique<BrowserManager>();

  client_state_manager_ = std::make_unique<ClientStateManager>();
  client_state_manager_->Initialize(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  confirmation_state_manager_ = std::make_unique<ConfirmationStateManager>();
  confirmation_state_manager_->Initialize(
      GetWalletForTesting(),
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  covariate_manager_ = std::make_unique<CovariateManager>();

  database_manager_ = std::make_unique<DatabaseManager>();
  database_manager_->CreateOrOpen(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  diagnostic_manager_ = std::make_unique<DiagnosticManager>();

  flag_manager_ = std::make_unique<FlagManager>();

  history_manager_ = std::make_unique<HistoryManager>();

  idle_detection_manager_ = std::make_unique<IdleDetectionManager>();

  locale_manager_ = std::make_unique<LocaleManager>();

  notification_ad_manager_ = std::make_unique<NotificationAdManager>();

  pref_manager_ = std::make_unique<PrefManager>();

  resource_manager_ = std::make_unique<ResourceManager>();

  tab_manager_ = std::make_unique<TabManager>();

  user_activity_manager_ = std::make_unique<UserActivityManager>();

  // Fast forward until no tasks remain to ensure "EnsureSqliteInitialized"
  // tasks have fired before running tests.
  task_environment_.FastForwardUntilNoTasksRemain();
}

void UnitTestBase::SetDefaultMocks() {
  MockBuildChannel(BuildChannelType::kRelease);

  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  MockIsNetworkConnectionAvailable(ads_client_mock_, true);

  MockIsBrowserActive(ads_client_mock_, true);
  MockIsBrowserInFullScreenMode(ads_client_mock_, false);

  MockCanShowNotificationAds(ads_client_mock_, true);
  MockCanShowNotificationAdsWhileBrowserIsBackgrounded(ads_client_mock_, false);
  MockShowNotificationAd(ads_client_mock_);
  MockCloseNotificationAd(ads_client_mock_);

  MockRecordAdEventForId(ads_client_mock_);
  MockGetAdEventHistory(ads_client_mock_);
  MockResetAdEventHistoryForId(ads_client_mock_);

  MockGetBrowsingHistory(ads_client_mock_);

  MockSave(ads_client_mock_);
  MockLoad(ads_client_mock_, temp_dir_);
  MockLoadFileResource(ads_client_mock_, temp_dir_);
  MockLoadDataResource(ads_client_mock_);

  const base::FilePath database_path =
      temp_dir_.GetPath().AppendASCII(kDatabaseFilename);
  database_ = std::make_unique<Database>(database_path);
  MockRunDBTransaction(ads_client_mock_, database_);

  MockGetBooleanPref(ads_client_mock_);
  MockSetBooleanPref(ads_client_mock_);
  MockGetIntegerPref(ads_client_mock_);
  MockSetIntegerPref(ads_client_mock_);
  MockGetDoublePref(ads_client_mock_);
  MockSetDoublePref(ads_client_mock_);
  MockGetStringPref(ads_client_mock_);
  MockSetStringPref(ads_client_mock_);
  MockGetInt64Pref(ads_client_mock_);
  MockSetInt64Pref(ads_client_mock_);
  MockGetUint64Pref(ads_client_mock_);
  MockSetUint64Pref(ads_client_mock_);
  MockGetTimePref(ads_client_mock_);
  MockSetTimePref(ads_client_mock_);
  MockGetDictPref(ads_client_mock_);
  MockSetDictPref(ads_client_mock_);
  MockGetListPref(ads_client_mock_);
  MockSetListPref(ads_client_mock_);
  MockClearPref(ads_client_mock_);
  MockHasPrefPath(ads_client_mock_);
}

void UnitTestBase::SetDefaultPrefs() {
  ads_client_mock_->SetBooleanPref(prefs::kEnabled, true);

  ads_client_mock_->SetStringPref(prefs::kDiagnosticId, "");

  ads_client_mock_->SetInt64Pref(prefs::kMaximumNotificationAdsPerHour, -1);

  ads_client_mock_->SetIntegerPref(prefs::kIdleTimeThreshold, 15);

  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   false);
  ads_client_mock_->SetStringPref(prefs::kSubdivisionTargetingCode, "AUTO");
  ads_client_mock_->SetStringPref(prefs::kAutoDetectedSubdivisionTargetingCode,
                                  "");

  ads_client_mock_->SetStringPref(prefs::kCatalogId, "");
  ads_client_mock_->SetIntegerPref(prefs::kCatalogVersion, 1);
  ads_client_mock_->SetInt64Pref(prefs::kCatalogPing, 7'200'000);
  ads_client_mock_->SetTimePref(prefs::kCatalogLastUpdated, DistantPast());

  ads_client_mock_->SetInt64Pref(prefs::kIssuerPing, 0);
  ads_client_mock_->SetListPref(prefs::kIssuers, base::Value::List());

  ads_client_mock_->SetDictPref(prefs::kEpsilonGreedyBanditArms,
                                base::Value::Dict());
  ads_client_mock_->SetListPref(prefs::kEpsilonGreedyBanditEligibleSegments,
                                base::Value::List());

  ads_client_mock_->SetListPref(prefs::kNotificationAds, base::Value::List());
  ads_client_mock_->SetTimePref(prefs::kServeAdAt, Now());

  ads_client_mock_->SetTimePref(prefs::kNextTokenRedemptionAt, DistantFuture());

  ads_client_mock_->SetBooleanPref(prefs::kHasMigratedClientState, true);
  ads_client_mock_->SetBooleanPref(prefs::kHasMigratedConfirmationState, true);
  ads_client_mock_->SetBooleanPref(prefs::kHasMigratedConversionState, true);
  ads_client_mock_->SetBooleanPref(prefs::kHasMigratedNotificationState, true);
  ads_client_mock_->SetBooleanPref(prefs::kHasMigratedRewardsState, true);

  ads_client_mock_->SetUint64Pref(prefs::kConfirmationsHash, 0);
  ads_client_mock_->SetUint64Pref(prefs::kClientHash, 0);

  ads_client_mock_->SetStringPref(prefs::kBrowserVersionNumber, "");
}

void UnitTestBase::SetUpIntegrationTest() {
  CHECK(is_integration_test_)
      << "|SetUpIntegrationTest| should only be called if |SetUpForTesting| is "
         "initialized for integration testing";

  ads_ = std::make_unique<AdsImpl>(ads_client_mock_.get());

  ads_->OnRewardsWalletDidChange(GetWalletPaymentIdForTesting(),
                                 GetWalletRecoverySeedForTesting());

  ads_->Initialize(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  task_environment_.RunUntilIdle();
}

}  // namespace ads
