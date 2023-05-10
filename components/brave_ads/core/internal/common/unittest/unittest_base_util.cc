/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base_util.h"

#include <cstdint>
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
#include "brave/components/brave_ads/core/database.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_test_suite_util.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

using ::testing::_;
using ::testing::Invoke;

namespace {

using AdEventHistoryMap =
    base::flat_map</*type_id*/ std::string, std::vector<base::Time>>;
using AdEventMap = base::flat_map</*uuid*/ std::string, AdEventHistoryMap>;

AdEventMap& AdEventHistory() {
  static base::NoDestructor<AdEventMap> ad_events;
  return *ad_events;
}

}  // namespace

PrefMap& Prefs() {
  static base::NoDestructor<PrefMap> prefs;
  return *prefs;
}

void MockShowNotificationAd(AdsClientMock& mock) {
  ON_CALL(mock, ShowNotificationAd(_))
      .WillByDefault(Invoke([](const NotificationAdInfo& ad) {
        // TODO(https://github.com/brave/brave-browser/issues/29587): Decouple
        // reminders from push notification ads.
        const bool is_reminder_valid = !ad.placement_id.empty() &&
                                       !ad.title.empty() && !ad.body.empty() &&
                                       ad.target_url.is_valid();

        CHECK(ad.IsValid() || is_reminder_valid);
      }));
}

void MockCloseNotificationAd(AdsClientMock& mock) {
  ON_CALL(mock, CloseNotificationAd(_))
      .WillByDefault(Invoke([](const std::string& placement_id) {
        CHECK(!placement_id.empty());
      }));
}

void MockRecordAdEventForId(const AdsClientMock& mock) {
  ON_CALL(mock, RecordAdEventForId(_, _, _, _))
      .WillByDefault(Invoke(
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
  ON_CALL(mock, GetAdEventHistory(_, _))
      .WillByDefault(Invoke(
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
  ON_CALL(mock, ResetAdEventHistoryForId(_))
      .WillByDefault(Invoke([](const std::string& id) {
        CHECK(!id.empty());

        const std::string uuid = GetUuidForCurrentTestAndValue(id);
        AdEventHistory()[uuid] = {};
      }));
}

void MockSave(AdsClientMock& mock) {
  ON_CALL(mock, Save(_, _, _))
      .WillByDefault(
          Invoke([](const std::string& /*name*/, const std::string& /*value*/,
                    SaveCallback callback) {
            std::move(callback).Run(/*success*/ true);
          }));
}

void MockLoad(AdsClientMock& mock, const base::ScopedTempDir& temp_dir) {
  ON_CALL(mock, Load(_, _))
      .WillByDefault(
          Invoke([&temp_dir](const std::string& name, LoadCallback callback) {
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
  ON_CALL(mock, LoadFileResource(_, _, _))
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

void MockLoadDataResource(AdsClientMock& mock) {
  ON_CALL(mock, LoadDataResource(_))
      .WillByDefault(Invoke([](const std::string& name) -> std::string {
        return ReadFileFromDataResourcePathToString(name).value_or("");
      }));
}

void MockRunDBTransaction(AdsClientMock& mock, Database& database) {
  ON_CALL(mock, RunDBTransaction(_, _))
      .WillByDefault(Invoke([&database](mojom::DBTransactionInfoPtr transaction,
                                        RunDBTransactionCallback callback) {
        CHECK(transaction);

        mojom::DBCommandResponseInfoPtr command_response =
            mojom::DBCommandResponseInfo::New();

        database.RunTransaction(std::move(transaction), &*command_response);

        std::move(callback).Run(std::move(command_response));
      }));
}

void MockGetBooleanPref(const AdsClientMock& mock) {
  ON_CALL(mock, GetBooleanPref(_))
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

void MockGetIntegerPref(const AdsClientMock& mock) {
  ON_CALL(mock, GetIntegerPref(_))
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

void MockGetDoublePref(const AdsClientMock& mock) {
  ON_CALL(mock, GetDoublePref(_))
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

void MockGetStringPref(const AdsClientMock& mock) {
  ON_CALL(mock, GetStringPref(_))
      .WillByDefault(Invoke([](const std::string& path) -> std::string {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        return Prefs()[uuid];
      }));
}

void MockGetInt64Pref(const AdsClientMock& mock) {
  ON_CALL(mock, GetInt64Pref(_))
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

void MockGetUint64Pref(const AdsClientMock& mock) {
  ON_CALL(mock, GetUint64Pref(_))
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

void MockGetTimePref(const AdsClientMock& mock) {
  ON_CALL(mock, GetTimePref(_))
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

void MockGetDictPref(const AdsClientMock& mock) {
  ON_CALL(mock, GetDictPref(_))
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

void MockGetListPref(const AdsClientMock& mock) {
  ON_CALL(mock, GetListPref(_))
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

void MockClearPref(AdsClientMock& mock) {
  ON_CALL(mock, ClearPref(_)).WillByDefault(Invoke([](const std::string& path) {
    const std::string uuid = GetUuidForCurrentTestAndValue(path);
    Prefs().erase(uuid);
  }));
}

void MockHasPrefPath(const AdsClientMock& mock) {
  ON_CALL(mock, HasPrefPath(_))
      .WillByDefault(Invoke([](const std::string& path) -> bool {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        return Prefs().find(uuid) != Prefs().cend();
      }));
}

}  // namespace brave_ads
