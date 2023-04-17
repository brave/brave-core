/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

#include <cstdint>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/extend.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/database.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_test_suite_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

using AdEventHistoryMap =
    base::flat_map</*type_id*/ std::string, std::vector<base::Time>>;
using AdEventMap = base::flat_map</*uuid*/ std::string, AdEventHistoryMap>;

using PrefMap = base::flat_map</*uuid*/ std::string, /*value*/ std::string>;

namespace {

AdEventMap& AdEventHistory() {
  static base::NoDestructor<AdEventMap> ad_events;
  return *ad_events;
}

PrefMap& Prefs() {
  static base::NoDestructor<PrefMap> prefs;
  return *prefs;
}

void MockShowNotificationAd(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, ShowNotificationAd(_))
      .WillByDefault(Invoke([](const NotificationAdInfo& ad) {
        // TODO(https://github.com/brave/brave-browser/issues/29587): Decouple
        // reminders from push notification ads.
        const bool is_reminder_valid = !ad.placement_id.empty() &&
                                       !ad.title.empty() && !ad.body.empty() &&
                                       ad.target_url.is_valid();

        CHECK(ad.IsValid() || is_reminder_valid);
      }));
}

void MockCloseNotificationAd(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, CloseNotificationAd(_))
      .WillByDefault(Invoke([](const std::string& placement_id) {
        CHECK(!placement_id.empty());
      }));
}

void MockRecordAdEventForId(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, RecordAdEventForId(_, _, _, _))
      .WillByDefault(Invoke(
          [](const std::string& id, const std::string& ad_type,
             const std::string& confirmation_type, const base::Time time) {
            CHECK(!id.empty());
            CHECK(!ad_type.empty());
            CHECK(!confirmation_type.empty());

            const std::string uuid = GetUuidForCurrentTestAndValue(id);
            const std::string type_id = ad_type + confirmation_type;
            AdEventHistory()[uuid][type_id].push_back(time);
          }));
}

void MockGetAdEventHistory(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetAdEventHistory(_, _))
      .WillByDefault(Invoke(
          [](const std::string& ad_type,
             const std::string& confirmation_type) -> std::vector<base::Time> {
            CHECK(!ad_type.empty());
            CHECK(!confirmation_type.empty());

            const std::string namespace_for_current_test =
                GetNamespaceForCurrentTest();

            const std::string type_id = ad_type + confirmation_type;

            std::vector<base::Time> timestamps;

            for (const auto& [uuid, ad_event_history] : AdEventHistory()) {
              if (!base::EndsWith(uuid, namespace_for_current_test,
                                  base::CompareCase::SENSITIVE)) {
                // Only get ad events for current test namespace.
                continue;
              }

              for (const auto& [ad_event_type_id, ad_event_timestamps] :
                   ad_event_history) {
                if (ad_event_type_id == type_id) {
                  base::Extend(timestamps, ad_event_timestamps);
                }
              }
            }

            return timestamps;
          }));
}

void MockResetAdEventHistoryForId(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, ResetAdEventHistoryForId(_))
      .WillByDefault(Invoke([](const std::string& id) {
        CHECK(!id.empty());

        const std::string uuid = GetUuidForCurrentTestAndValue(id);
        AdEventHistory()[uuid] = {};
      }));
}

void MockSave(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, Save(_, _, _))
      .WillByDefault(
          Invoke([](const std::string& /*name*/, const std::string& /*value*/,
                    SaveCallback callback) {
            std::move(callback).Run(/*success*/ true);
          }));
}

void MockLoad(const std::unique_ptr<AdsClientMock>& mock,
              const base::ScopedTempDir& temp_dir) {
  ON_CALL(*mock, Load(_, _))
      .WillByDefault(
          Invoke([&temp_dir](const std::string& name, LoadCallback callback) {
            base::FilePath path = temp_dir.GetPath().AppendASCII(name);
            if (!base::PathExists(path)) {
              // If path does not exist load the file from the test path.
              path = GetTestPath().AppendASCII(name);
            }

            std::string value;
            if (!base::ReadFileToString(path, &value)) {
              return std::move(callback).Run(/*success*/ false, value);
            }

            std::move(callback).Run(/*success*/ true, value);
          }));
}

void MockLoadFileResource(const std::unique_ptr<AdsClientMock>& mock,
                          const base::ScopedTempDir& temp_dir) {
  ON_CALL(*mock, LoadFileResource(_, _, _))
      .WillByDefault(
          Invoke([&temp_dir](const std::string& id, const int /*version*/,
                             LoadFileCallback callback) {
            base::FilePath path = temp_dir.GetPath().AppendASCII(id);

            if (!base::PathExists(path)) {
              // If path does not exist load the file from the test path.
              path = GetFileResourcePath().AppendASCII(id);
            }

            base::File file(path, base::File::Flags::FLAG_OPEN |
                                      base::File::Flags::FLAG_READ);
            std::move(callback).Run(std::move(file));
          }));
}

void MockLoadDataResource(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, LoadDataResource(_))
      .WillByDefault(Invoke([](const std::string& name) -> std::string {
        return ReadFileFromDataResourcePathToString(name).value_or("");
      }));
}

void MockRunDBTransaction(const std::unique_ptr<AdsClientMock>& mock,
                          const std::unique_ptr<Database>& database) {
  ON_CALL(*mock, RunDBTransaction(_, _))
      .WillByDefault(Invoke([&database](mojom::DBTransactionInfoPtr transaction,
                                        RunDBTransactionCallback callback) {
        CHECK(transaction);

        mojom::DBCommandResponseInfoPtr command_response =
            mojom::DBCommandResponseInfo::New();

        if (!database) {
          command_response->status =
              mojom::DBCommandResponseInfo::StatusType::RESPONSE_ERROR;
        } else {
          database->RunTransaction(std::move(transaction),
                                   command_response.get());
        }

        std::move(callback).Run(std::move(command_response));
      }));
}

void MockGetBooleanPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetBooleanPref(_))
      .WillByDefault(Invoke([](const std::string& path) -> bool {
        int value = 0;
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        const std::string& value_as_string = Prefs()[uuid];
        if (!value_as_string.empty()) {
          CHECK(base::StringToInt(value_as_string, &value));
        }
        return static_cast<bool>(value);
      }));
}

void MockGetIntegerPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetIntegerPref(_))
      .WillByDefault(Invoke([](const std::string& path) -> int {
        int value = 0;
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        const std::string& value_as_string = Prefs()[uuid];
        if (!value_as_string.empty()) {
          CHECK(base::StringToInt(value_as_string, &value));
        }
        return value;
      }));
}

void MockGetDoublePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetDoublePref(_))
      .WillByDefault(Invoke([](const std::string& path) -> double {
        double value = 0.0;
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        const std::string& value_as_string = Prefs()[uuid];
        if (!value_as_string.empty()) {
          CHECK(base::StringToDouble(value_as_string, &value));
        }
        return value;
      }));
}

void MockGetStringPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetStringPref(_))
      .WillByDefault(Invoke([](const std::string& path) -> std::string {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        return Prefs()[uuid];
      }));
}

void MockGetInt64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetInt64Pref(_))
      .WillByDefault(Invoke([](const std::string& path) -> int64_t {
        int64_t value = 0;
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        const std::string& value_as_string = Prefs()[uuid];
        if (!value_as_string.empty()) {
          CHECK(base::StringToInt64(value_as_string, &value));
        }
        return value;
      }));
}

void MockGetUint64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetUint64Pref(_))
      .WillByDefault(Invoke([](const std::string& path) -> uint64_t {
        uint64_t value = 0;
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        const std::string& value_as_string = Prefs()[uuid];
        if (!value_as_string.empty()) {
          CHECK(base::StringToUint64(value_as_string, &value));
        }
        return value;
      }));
}

void MockGetTimePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetTimePref(_))
      .WillByDefault(Invoke([](const std::string& path) -> base::Time {
        int64_t value = 0;
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        const std::string& value_as_string = Prefs()[uuid];
        if (!value_as_string.empty()) {
          CHECK(base::StringToInt64(value_as_string, &value));
        }
        return base::Time::FromDeltaSinceWindowsEpoch(
            base::Microseconds(value));
      }));
}

void MockGetDictPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetDictPref(_))
      .WillByDefault(Invoke(
          [](const std::string& path) -> absl::optional<base::Value::Dict> {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            const std::string& json = Prefs()[uuid];
            const absl::optional<base::Value> root =
                base::JSONReader::Read(json);
            if (!root) {
              return absl::nullopt;
            }

            const base::Value::Dict* const dict = root->GetIfDict();
            CHECK(dict);
            return dict->Clone();
          }));
}

void MockGetListPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetListPref(_))
      .WillByDefault(Invoke(
          [](const std::string& path) -> absl::optional<base::Value::List> {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            const std::string& json = Prefs()[uuid];
            const absl::optional<base::Value> root =
                base::JSONReader::Read(json);
            if (!root) {
              return absl::nullopt;
            }

            const base::Value::List* const list = root->GetIfList();
            CHECK(list);
            return list->Clone();
          }));
}

void MockClearPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, ClearPref(_))
      .WillByDefault(Invoke([](const std::string& path) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs().erase(uuid);
      }));
}

void MockHasPrefPath(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, HasPrefPath(_))
      .WillByDefault(Invoke([](const std::string& path) -> bool {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        return Prefs().find(uuid) != Prefs().cend();
      }));
}

}  // namespace

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

  MockDefaultAdsClient();

  MockDefaultPrefs();

  if (!is_integration_test_) {
    ads_client_helper_ =
        std::make_unique<AdsClientHelper>(ads_client_mock_.get());
  }

  SetUpMocks();

  if (is_integration_test_) {
    return SetUpIntegrationTest();
  }

  browser_manager_ = std::make_unique<BrowserManager>();

  client_state_manager_ = std::make_unique<ClientStateManager>();
  client_state_manager_->Initialize(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  confirmation_state_manager_ = std::make_unique<ConfirmationStateManager>();
  confirmation_state_manager_->Initialize(
      GetWalletForTesting(),  // IN-TEST
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  database_manager_ = std::make_unique<DatabaseManager>();
  database_manager_->CreateOrOpen(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  diagnostic_manager_ = std::make_unique<DiagnosticManager>();

  flag_manager_ = std::make_unique<FlagManager>();

  history_manager_ = std::make_unique<HistoryManager>();

  idle_detection_ = std::make_unique<IdleDetection>();

  notification_ad_manager_ = std::make_unique<NotificationAdManager>();

  predictors_manager_ = std::make_unique<PredictorsManager>();

  tab_manager_ = std::make_unique<TabManager>();

  user_activity_manager_ = std::make_unique<UserActivityManager>();

  // Fast forward until no tasks remain to ensure "EnsureSqliteInitialized"
  // tasks have fired before running tests.
  task_environment_.FastForwardUntilNoTasksRemain();
}

void UnitTestBase::MockAdsClientAddObserver() {
  ON_CALL(*ads_client_mock_, AddObserver(_))
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

  MockBuildChannel(BuildChannelType::kRelease);

  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  MockIsNetworkConnectionAvailable(ads_client_mock_, true);

  MockIsBrowserActive(ads_client_mock_, true);
  MockIsBrowserInFullScreenMode(ads_client_mock_, false);

  MockCanShowNotificationAds(ads_client_mock_, true);
  MockCanShowNotificationAdsWhileBrowserIsBackgrounded(ads_client_mock_, false);

  MockGetBrowsingHistory(ads_client_mock_);
}

void UnitTestBase::MockSetBooleanPref(
    const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetBooleanPref(_, _))
      .WillByDefault(Invoke([&](const std::string& path, const bool value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(static_cast<int>(value));
        NotifyPrefDidChange(path);
      }));
}

void UnitTestBase::MockSetIntegerPref(
    const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetIntegerPref(_, _))
      .WillByDefault(Invoke([&](const std::string& path, const int value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
        NotifyPrefDidChange(path);
      }));
}

void UnitTestBase::MockSetDoublePref(
    const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetDoublePref(_, _))
      .WillByDefault(Invoke([&](const std::string& path, const double value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
        NotifyPrefDidChange(path);
      }));
}

void UnitTestBase::MockSetStringPref(
    const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetStringPref(_, _))
      .WillByDefault(
          Invoke([&](const std::string& path, const std::string& value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            Prefs()[uuid] = value;
            NotifyPrefDidChange(path);
          }));
}

void UnitTestBase::MockSetInt64Pref(
    const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetInt64Pref(_, _))
      .WillByDefault(Invoke([&](const std::string& path, const int64_t value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
        NotifyPrefDidChange(path);
      }));
}

void UnitTestBase::MockSetUint64Pref(
    const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetUint64Pref(_, _))
      .WillByDefault(Invoke([&](const std::string& path, const uint64_t value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
        NotifyPrefDidChange(path);
      }));
}

void UnitTestBase::MockSetDictPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetDictPref(_, _))
      .WillByDefault(
          Invoke([&](const std::string& path, base::Value::Dict value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            std::string json;
            CHECK(base::JSONWriter::Write(value, &json));
            Prefs()[uuid] = json;
            NotifyPrefDidChange(path);
          }));
}

void UnitTestBase::MockSetListPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetListPref(_, _))
      .WillByDefault(
          Invoke([&](const std::string& path, base::Value::List value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            std::string json;
            CHECK(base::JSONWriter::Write(value, &json));
            Prefs()[uuid] = json;
            NotifyPrefDidChange(path);
          }));
}

void UnitTestBase::MockSetTimePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetTimePref(_, _))
      .WillByDefault(
          Invoke([&](const std::string& path, const base::Time value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            Prefs()[uuid] = base::NumberToString(
                value.ToDeltaSinceWindowsEpoch().InMicroseconds());
            NotifyPrefDidChange(path);
          }));
}

void UnitTestBase::MockDefaultPrefs() {
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
  ads_client_mock_->SetBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser,
                                   false);

  ads_client_mock_->SetUint64Pref(prefs::kConfirmationsHash, 0);
  ads_client_mock_->SetUint64Pref(prefs::kClientHash, 0);

  ads_client_mock_->SetStringPref(prefs::kBrowserVersionNumber, "");
}

void UnitTestBase::SetUpIntegrationTest() {
  CHECK(is_integration_test_)
      << "|SetUpIntegrationTest| should only be called if |SetUpForTesting| is "
         "initialized for integration testing";

  ads_ = std::make_unique<AdsImpl>(ads_client_mock_.get());

  ads_->OnRewardsWalletDidChange(kWalletPaymentId, kWalletRecoverySeed);

  ads_->Initialize(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  task_environment_.RunUntilIdle();
}

}  // namespace brave_ads
