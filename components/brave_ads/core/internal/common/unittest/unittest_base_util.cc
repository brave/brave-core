/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base_util.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/extend.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_current_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/database/database.h"
#include "brave/components/brave_ads/core/public/flags/flags_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

using AdEventHistoryMap =
    base::flat_map</*type_id*/ std::string, std::vector<base::Time>>;
using AdEventMap = base::flat_map</*uuid*/ std::string, AdEventHistoryMap>;

AdEventMap& AdEventHistory() {
  static base::NoDestructor<AdEventMap> ad_events;
  return *ad_events;
}

}  // namespace

void MockFlags() {
  GlobalState::GetInstance()->Flags() = *BuildFlags();

  // Use the staging environment for tests if we did not append command line
  // switches in |SetUpMocks|.
  if (!DidAppendCommandLineSwitches()) {
    CHECK(GlobalState::HasInstance());
    GlobalState::GetInstance()->Flags().environment_type =
        mojom::EnvironmentType::kStaging;
  }
}

void MockShowNotificationAd(AdsClientMock& mock) {
  ON_CALL(mock, ShowNotificationAd)
      .WillByDefault(::testing::Invoke([](const NotificationAdInfo& ad) {
        // TODO(https://github.com/brave/brave-browser/issues/29587): Decouple
        // reminders from push notification ads.
        const bool is_reminder_valid = !ad.placement_id.empty() &&
                                       !ad.title.empty() && !ad.body.empty() &&
                                       ad.target_url.is_valid();

        CHECK(ad.IsValid() || is_reminder_valid);
      }));
}

void MockCloseNotificationAd(AdsClientMock& mock) {
  ON_CALL(mock, CloseNotificationAd)
      .WillByDefault(::testing::Invoke([](const std::string& placement_id) {
        CHECK(!placement_id.empty());
      }));
}

void MockRecordAdEventForId(const AdsClientMock& mock) {
  ON_CALL(mock, RecordAdEventForId)
      .WillByDefault(::testing::Invoke(
          [](const std::string& id, const std::string& ad_type,
             const std::string& confirmation_type, const base::Time time) {
            CHECK(!id.empty());
            CHECK(!ad_type.empty());
            CHECK(!confirmation_type.empty());

            const std::string uuid = GetUuidForCurrentTestAndValue(id);
            const std::string type_id =
                base::StrCat({ad_type, confirmation_type});
            AdEventHistory()[uuid][type_id].push_back(time);
          }));
}

void MockGetAdEventHistory(const AdsClientMock& mock) {
  ON_CALL(mock, GetAdEventHistory)
      .WillByDefault(::testing::Invoke(
          [](const std::string& ad_type,
             const std::string& confirmation_type) -> std::vector<base::Time> {
            CHECK(!ad_type.empty());
            CHECK(!confirmation_type.empty());

            const std::string uuid_for_current_test = GetUuidForCurrentTest();

            const std::string type_id =
                base::StrCat({ad_type, confirmation_type});

            std::vector<base::Time> ad_event_history;

            for (const auto& [uuid, history] : AdEventHistory()) {
              if (!base::EndsWith(uuid,
                                  base::StrCat({":", uuid_for_current_test}),
                                  base::CompareCase::SENSITIVE)) {
                // Only get ad events for current test.
                continue;
              }

              for (const auto& [ad_event_type_id, timestamps] : history) {
                if (ad_event_type_id == type_id) {
                  base::Extend(ad_event_history, timestamps);
                }
              }
            }

            return ad_event_history;
          }));
}

void MockResetAdEventHistoryForId(const AdsClientMock& mock) {
  ON_CALL(mock, ResetAdEventHistoryForId)
      .WillByDefault(::testing::Invoke([](const std::string& id) {
        CHECK(!id.empty());

        const std::string uuid = GetUuidForCurrentTestAndValue(id);
        AdEventHistory()[uuid].erase(uuid);
      }));
}

void MockSave(AdsClientMock& mock) {
  ON_CALL(mock, Save)
      .WillByDefault(::testing::Invoke([](const std::string& /*name*/,
                                          const std::string& /*value*/,
                                          SaveCallback callback) {
        std::move(callback).Run(/*success*/ true);
      }));
}

void MockLoad(AdsClientMock& mock, const base::ScopedTempDir& temp_dir) {
  ON_CALL(mock, Load)
      .WillByDefault(::testing::Invoke(
          [&temp_dir](const std::string& name, LoadCallback callback) {
            base::FilePath path = temp_dir.GetPath().AppendASCII(name);
            if (!base::PathExists(path)) {
              // If path does not exist load the file from the test path.
              path = GetTestPath().AppendASCII(name);
            }

            std::string value;
            if (!base::ReadFileToString(path, &value)) {
              return std::move(callback).Run(absl::nullopt);
            }

            std::move(callback).Run(value);
          }));
}

void MockLoadFileResource(AdsClientMock& mock,
                          const base::ScopedTempDir& temp_dir) {
  ON_CALL(mock, LoadFileResource)
      .WillByDefault(::testing::Invoke(
          [&temp_dir](const std::string& id, const int /*version*/,
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

void MockLoadDataResource(AdsClientMock& mock) {
  ON_CALL(mock, LoadDataResource)
      .WillByDefault(
          ::testing::Invoke([](const std::string& name) -> std::string {
            return ReadFileFromDataResourcePathToString(name).value_or("");
          }));
}

void MockRunDBTransaction(AdsClientMock& mock, Database& database) {
  ON_CALL(mock, RunDBTransaction)
      .WillByDefault(
          ::testing::Invoke([&database](mojom::DBTransactionInfoPtr transaction,
                                        RunDBTransactionCallback callback) {
            CHECK(transaction);

            mojom::DBCommandResponseInfoPtr command_response =
                mojom::DBCommandResponseInfo::New();

            database.RunTransaction(std::move(transaction), &*command_response);

            std::move(callback).Run(std::move(command_response));
          }));
}

void MockGetBooleanPref(const AdsClientMock& mock) {
  ON_CALL(mock, GetBooleanPref)
      .WillByDefault(::testing::Invoke([](const std::string& path) -> bool {
        int value;
        CHECK(base::StringToInt(GetPrefValue(path), &value));
        return static_cast<bool>(value);
      }));
}

void MockGetIntegerPref(const AdsClientMock& mock) {
  ON_CALL(mock, GetIntegerPref)
      .WillByDefault(::testing::Invoke([](const std::string& path) -> int {
        int value;
        CHECK(base::StringToInt(GetPrefValue(path), &value));
        return value;
      }));
}

void MockGetDoublePref(const AdsClientMock& mock) {
  ON_CALL(mock, GetDoublePref)
      .WillByDefault(::testing::Invoke([](const std::string& path) -> double {
        double value;
        CHECK(base::StringToDouble(GetPrefValue(path), &value));
        return value;
      }));
}

void MockGetStringPref(const AdsClientMock& mock) {
  ON_CALL(mock, GetStringPref)
      .WillByDefault(
          ::testing::Invoke([](const std::string& path) -> std::string {
            return GetPrefValue(path);
          }));
}

void MockGetInt64Pref(const AdsClientMock& mock) {
  ON_CALL(mock, GetInt64Pref)
      .WillByDefault(::testing::Invoke([](const std::string& path) -> int64_t {
        int64_t value;
        CHECK(base::StringToInt64(GetPrefValue(path), &value));
        return value;
      }));
}

void MockGetUint64Pref(const AdsClientMock& mock) {
  ON_CALL(mock, GetUint64Pref)
      .WillByDefault(::testing::Invoke([](const std::string& path) -> uint64_t {
        uint64_t value;
        CHECK(base::StringToUint64(GetPrefValue(path), &value));
        return value;
      }));
}

void MockGetTimePref(const AdsClientMock& mock) {
  ON_CALL(mock, GetTimePref)
      .WillByDefault(
          ::testing::Invoke([](const std::string& path) -> base::Time {
            int64_t value;
            CHECK(base::StringToInt64(GetPrefValue(path), &value));
            return base::Time::FromDeltaSinceWindowsEpoch(
                base::Microseconds(value));
          }));
}

void MockGetDictPref(const AdsClientMock& mock) {
  ON_CALL(mock, GetDictPref)
      .WillByDefault(::testing::Invoke(
          [](const std::string& path) -> absl::optional<base::Value::Dict> {
            const absl::optional<base::Value> root =
                base::JSONReader::Read(GetPrefValue(path));
            if (!root) {
              return absl::nullopt;
            }

            const base::Value::Dict* const dict = root->GetIfDict();
            CHECK(dict);
            return dict->Clone();
          }));
}

void MockGetListPref(const AdsClientMock& mock) {
  ON_CALL(mock, GetListPref)
      .WillByDefault(::testing::Invoke(
          [](const std::string& path) -> absl::optional<base::Value::List> {
            const absl::optional<base::Value> root =
                base::JSONReader::Read(GetPrefValue(path));
            if (!root) {
              return absl::nullopt;
            }

            const base::Value::List* const list = root->GetIfList();
            CHECK(list);
            return list->Clone();
          }));
}

void MockClearPref(AdsClientMock& mock) {
  ON_CALL(mock, ClearPref)
      .WillByDefault(::testing::Invoke(
          [](const std::string& path) { ClearPrefValue(path); }));
}

void MockHasPrefPath(const AdsClientMock& mock) {
  ON_CALL(mock, HasPrefPath)
      .WillByDefault(::testing::Invoke([](const std::string& path) -> bool {
        return HasPrefPathValue(path);
      }));
}

}  // namespace brave_ads
