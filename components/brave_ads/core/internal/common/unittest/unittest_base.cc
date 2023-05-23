/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

#include <cstdint>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/database.h"
#include "brave/components/brave_ads/core/flags_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_current_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

using ::testing::_;
using ::testing::Invoke;

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

  MockDefaultAdsClient();

  MockDefaultPrefs();

  SetUpMocks();

  if (is_integration_test_) {
    return SetUpIntegrationTest();
  }

  global_state_ = std::make_unique<GlobalState>(&ads_client_mock_);

  MockBuildChannel(BuildChannelType::kRelease);

  global_state_->Flags() = *BuildFlags();

  global_state_->GetClientStateManager().Load(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  global_state_->GetConfirmationStateManager().Load(
      GetWalletForTesting(),  // IN-TEST
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  global_state_->GetDatabaseManager().CreateOrOpen(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  // Fast forward until no tasks remain to ensure
  // "EnsureSqliteInitialized" tasks have fired before running tests.
  task_environment_.FastForwardUntilNoTasksRemain();
}

void UnitTestBase::MockAdsClientAddObserver() {
  ON_CALL(ads_client_mock_, AddObserver(_))
      .WillByDefault(Invoke(
          [=](AdsClientNotifierObserver* observer) { AddObserver(observer); }));
}

void UnitTestBase::MockDefaultAdsClient() {
  MockAdsClientAddObserver();

  MockShowNotificationAd(ads_client_mock_);
  MockCloseNotificationAd(ads_client_mock_);

  MockRecordAdEventForId(ads_client_mock_);
  MockGetAdEventHistory(ads_client_mock_);
  MockResetAdEventHistoryForId(ads_client_mock_);

  MockSave(ads_client_mock_);
  MockLoad(ads_client_mock_, temp_dir_);
  MockLoadFileResource(ads_client_mock_, temp_dir_);
  MockLoadDataResource(ads_client_mock_);

  database_ = std::make_unique<Database>(
      temp_dir_.GetPath().AppendASCII(kDatabaseFilename));
  MockRunDBTransaction(ads_client_mock_, *database_);

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

  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  MockIsNetworkConnectionAvailable(ads_client_mock_, true);

  MockIsBrowserActive(ads_client_mock_, true);
  MockIsBrowserInFullScreenMode(ads_client_mock_, false);

  MockCanShowNotificationAds(ads_client_mock_, true);
  MockCanShowNotificationAdsWhileBrowserIsBackgrounded(ads_client_mock_, false);

  MockGetBrowsingHistory(ads_client_mock_, /*history*/ {});
}

void UnitTestBase::MockSetBooleanPref(AdsClientMock& mock) {
  ON_CALL(mock, SetBooleanPref(_, _))
      .WillByDefault(Invoke([=](const std::string& path, const bool value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(static_cast<int>(value));
        NotifyPrefDidChange(path);
      }));
}

void UnitTestBase::MockSetIntegerPref(AdsClientMock& mock) {
  ON_CALL(mock, SetIntegerPref(_, _))
      .WillByDefault(Invoke([=](const std::string& path, const int value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
        NotifyPrefDidChange(path);
      }));
}

void UnitTestBase::MockSetDoublePref(AdsClientMock& mock) {
  ON_CALL(mock, SetDoublePref(_, _))
      .WillByDefault(Invoke([=](const std::string& path, const double value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
        NotifyPrefDidChange(path);
      }));
}

void UnitTestBase::MockSetStringPref(AdsClientMock& mock) {
  ON_CALL(mock, SetStringPref(_, _))
      .WillByDefault(
          Invoke([=](const std::string& path, const std::string& value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            Prefs()[uuid] = value;
            NotifyPrefDidChange(path);
          }));
}

void UnitTestBase::MockSetInt64Pref(AdsClientMock& mock) {
  ON_CALL(mock, SetInt64Pref(_, _))
      .WillByDefault(Invoke([=](const std::string& path, const int64_t value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
        NotifyPrefDidChange(path);
      }));
}

void UnitTestBase::MockSetUint64Pref(AdsClientMock& mock) {
  ON_CALL(mock, SetUint64Pref(_, _))
      .WillByDefault(Invoke([=](const std::string& path, const uint64_t value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
        NotifyPrefDidChange(path);
      }));
}

void UnitTestBase::MockSetDictPref(AdsClientMock& mock) {
  ON_CALL(mock, SetDictPref(_, _))
      .WillByDefault(
          Invoke([=](const std::string& path, base::Value::Dict value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            CHECK(base::JSONWriter::Write(value, &Prefs()[uuid]));
            NotifyPrefDidChange(path);
          }));
}

void UnitTestBase::MockSetListPref(AdsClientMock& mock) {
  ON_CALL(mock, SetListPref(_, _))
      .WillByDefault(
          Invoke([=](const std::string& path, base::Value::List value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            CHECK(base::JSONWriter::Write(value, &Prefs()[uuid]));
            NotifyPrefDidChange(path);
          }));
}

void UnitTestBase::MockSetTimePref(AdsClientMock& mock) {
  ON_CALL(mock, SetTimePref(_, _))
      .WillByDefault(
          Invoke([=](const std::string& path, const base::Time value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            Prefs()[uuid] = base::NumberToString(
                value.ToDeltaSinceWindowsEpoch().InMicroseconds());
            NotifyPrefDidChange(path);
          }));
}

void UnitTestBase::MockDefaultPrefs() {
  ads_client_mock_.SetBooleanPref(prefs::kEnabled, true);

  ads_client_mock_.SetBooleanPref(brave_news::prefs::kBraveNewsOptedIn, true);
  ads_client_mock_.SetBooleanPref(brave_news::prefs::kNewTabPageShowToday,
                                  true);

  ads_client_mock_.SetStringPref(prefs::kDiagnosticId, "");

  ads_client_mock_.SetInt64Pref(prefs::kMaximumNotificationAdsPerHour, -1);

  ads_client_mock_.SetIntegerPref(prefs::kIdleTimeThreshold, 15);

  ads_client_mock_.SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                  false);
  ads_client_mock_.SetStringPref(prefs::kSubdivisionTargetingCode, "AUTO");
  ads_client_mock_.SetStringPref(prefs::kAutoDetectedSubdivisionTargetingCode,
                                 "");

  ads_client_mock_.SetStringPref(prefs::kCatalogId, "");
  ads_client_mock_.SetIntegerPref(prefs::kCatalogVersion, 1);
  ads_client_mock_.SetInt64Pref(prefs::kCatalogPing, 7'200'000);
  ads_client_mock_.SetTimePref(prefs::kCatalogLastUpdated, DistantPast());

  ads_client_mock_.SetInt64Pref(prefs::kIssuerPing, 0);
  ads_client_mock_.SetListPref(prefs::kIssuers, base::Value::List());

  ads_client_mock_.SetDictPref(prefs::kEpsilonGreedyBanditArms,
                               base::Value::Dict());
  ads_client_mock_.SetListPref(prefs::kEpsilonGreedyBanditEligibleSegments,
                               base::Value::List());

  ads_client_mock_.SetListPref(prefs::kNotificationAds, base::Value::List());
  ads_client_mock_.SetTimePref(prefs::kServeAdAt, Now());

  ads_client_mock_.SetTimePref(prefs::kNextTokenRedemptionAt, DistantFuture());

  ads_client_mock_.SetBooleanPref(prefs::kHasMigratedClientState, true);
  ads_client_mock_.SetBooleanPref(prefs::kHasMigratedConfirmationState, true);
  ads_client_mock_.SetBooleanPref(prefs::kHasMigratedConversionState, true);
  ads_client_mock_.SetBooleanPref(prefs::kHasMigratedNotificationState, true);
  ads_client_mock_.SetBooleanPref(prefs::kHasMigratedRewardsState, true);
  ads_client_mock_.SetBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser,
                                  false);

  ads_client_mock_.SetUint64Pref(prefs::kConfirmationsHash, 0);
  ads_client_mock_.SetUint64Pref(prefs::kClientHash, 0);

  ads_client_mock_.SetStringPref(prefs::kBrowserVersionNumber, "");
}

void UnitTestBase::SetUpIntegrationTest() {
  CHECK(is_integration_test_)
      << "|SetUpIntegrationTest| should only be called if |SetUpForTesting| is "
         "initialized for integration testing";

  ads_ = std::make_unique<AdsImpl>(&ads_client_mock_);

  MockBuildChannel(BuildChannelType::kRelease);

  ads_->SetFlags(BuildFlags());

  ads_->OnRewardsWalletDidChange(kWalletPaymentId, kWalletRecoverySeed);

  ads_->Initialize(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  task_environment_.RunUntilIdle();
}

}  // namespace brave_ads
