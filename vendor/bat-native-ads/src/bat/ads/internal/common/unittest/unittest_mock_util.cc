/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/unittest/unittest_mock_util.h"

#include <cstdint>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/build_channel.h"
#include "bat/ads/database.h"
#include "bat/ads/internal/common/unittest/unittest_file_util.h"
#include "bat/ads/internal/common/unittest/unittest_test_suite_util.h"
#include "bat/ads/internal/common/unittest/unittest_url_response_util.h"
#include "bat/ads/notification_ad_info.h"
#include "url/gurl.h"

namespace ads {

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

using AdEventHistoryMap = base::flat_map<std::string, std::vector<base::Time>>;
using AdEventMap = base::flat_map<std::string, AdEventHistoryMap>;

using PrefMap = base::flat_map<std::string, std::string>;

namespace {

AdEventMap& AdEventHistory() {
  static base::NoDestructor<AdEventMap> ad_events;
  return *ad_events;
}

PrefMap& Prefs() {
  static base::NoDestructor<PrefMap> prefs;
  return *prefs;
}

}  // namespace

void MockBuildChannel(const BuildChannelType type) {
  switch (type) {
    case BuildChannelType::kNightly: {
      BuildChannel().is_release = false;
      BuildChannel().name = "nightly";
      return;
    }

    case BuildChannelType::kBeta: {
      BuildChannel().is_release = false;
      BuildChannel().name = "beta";
      return;
    }

    case BuildChannelType::kRelease: {
      BuildChannel().is_release = true;
      BuildChannel().name = "release";
      return;
    }
  }

  NOTREACHED() << "Unexpected value for BuildChannelType: "
               << static_cast<int>(type);
}

void MockPlatformHelper(const std::unique_ptr<PlatformHelperMock>& mock,
                        const PlatformType type) {
  PlatformHelper::SetForTesting(mock.get());

  bool is_mobile = false;
  std::string name;

  switch (type) {
    case PlatformType::kUnknown: {
      is_mobile = false;
      name = "unknown";
      break;
    }

    case PlatformType::kAndroid: {
      is_mobile = true;
      name = "android";
      break;
    }

    case PlatformType::kIOS: {
      is_mobile = true;
      name = "ios";
      break;
    }

    case PlatformType::kLinux: {
      is_mobile = false;
      name = "linux";
      break;
    }

    case PlatformType::kMacOS: {
      is_mobile = false;
      name = "macos";
      break;
    }

    case PlatformType::kWindows: {
      is_mobile = false;
      name = "windows";
      break;
    }
  }

  ON_CALL(*mock, IsMobile()).WillByDefault(Return(is_mobile));
  ON_CALL(*mock, GetName()).WillByDefault(Return(name));
  ON_CALL(*mock, GetType()).WillByDefault(Return(type));
}

void MockIsNetworkConnectionAvailable(
    const std::unique_ptr<AdsClientMock>& mock,
    const bool is_available) {
  ON_CALL(*mock, IsNetworkConnectionAvailable())
      .WillByDefault(Return(is_available));
}

void MockIsBrowserActive(const std::unique_ptr<AdsClientMock>& mock,
                         const bool is_browser_active) {
  ON_CALL(*mock, IsBrowserActive()).WillByDefault(Return(is_browser_active));
}

void MockIsBrowserInFullScreenMode(const std::unique_ptr<AdsClientMock>& mock,
                                   const bool is_browser_in_full_screen_mode) {
  ON_CALL(*mock, IsBrowserInFullScreenMode())
      .WillByDefault(Return(is_browser_in_full_screen_mode));
}

void MockCanShowNotificationAds(const std::unique_ptr<AdsClientMock>& mock,
                                const bool can_show) {
  ON_CALL(*mock, CanShowNotificationAds()).WillByDefault(Return(can_show));
}

void MockCanShowNotificationAdsWhileBrowserIsBackgrounded(
    const std::unique_ptr<AdsClientMock>& mock,
    const bool can_show) {
  ON_CALL(*mock, CanShowNotificationAdsWhileBrowserIsBackgrounded())
      .WillByDefault(Return(can_show));
}

void MockShowNotificationAd(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, ShowNotificationAd(_))
      .WillByDefault(
          Invoke([](const NotificationAdInfo& ad) { CHECK(ad.IsValid()); }));
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

            for (const auto& ad_event : AdEventHistory()) {
              const std::string& uuid = ad_event.first;
              if (!base::EndsWith(uuid, namespace_for_current_test,
                                  base::CompareCase::SENSITIVE)) {
                // Only get ad events for current test namespace.
                continue;
              }

              const AdEventHistoryMap& ad_event_history = ad_event.second;
              for (const auto& ad_event_history_item : ad_event_history) {
                const std::string& ad_event_type_id =
                    ad_event_history_item.first;
                if (ad_event_type_id != type_id) {
                  continue;
                }

                const std::vector<base::Time>& ad_event_timestamps =
                    ad_event_history_item.second;

                timestamps.insert(timestamps.cend(),
                                  ad_event_timestamps.cbegin(),
                                  ad_event_timestamps.cend());
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

void MockGetBrowsingHistory(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetBrowsingHistory(_, _, _))
      .WillByDefault(Invoke([](const int max_count, const int /*days_ago*/,
                               GetBrowsingHistoryCallback callback) {
        std::vector<GURL> history;

        for (int i = 0; i < max_count; i++) {
          const std::string spec =
              base::StringPrintf("https://www.brave.com/%d", i);
          history.emplace_back(spec);
        }

        std::move(callback).Run(history);
      }));
}

void MockUrlResponses(const std::unique_ptr<AdsClientMock>& mock,
                      const URLResponseMap& url_responses) {
  ON_CALL(*mock, UrlRequest(_, _))
      .WillByDefault(
          Invoke([url_responses](const mojom::UrlRequestInfoPtr& url_request,
                                 UrlRequestCallback callback) {
            const absl::optional<mojom::UrlResponseInfo> url_response =
                GetNextUrlResponseForRequest(url_request, url_responses);
            if (!url_response) {
              // URL request should not be mocked.
              std::move(callback).Run({});
              return;
            }

            std::move(callback).Run(*url_response);
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
              std::move(callback).Run(/*success*/ false, value);
              return;
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

        mojom::DBCommandResponseInfoPtr response =
            mojom::DBCommandResponseInfo::New();

        if (!database) {
          response->status =
              mojom::DBCommandResponseInfo::StatusType::RESPONSE_ERROR;
        } else {
          database->RunTransaction(std::move(transaction), response.get());
        }

        std::move(callback).Run(std::move(response));
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

void MockSetBooleanPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetBooleanPref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const bool value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(int{value});
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

void MockSetIntegerPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetIntegerPref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const int value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
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

void MockSetDoublePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetDoublePref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const double value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
      }));
}

void MockGetStringPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetStringPref(_))
      .WillByDefault(Invoke([](const std::string& path) -> std::string {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        return Prefs()[uuid];
      }));
}

void MockSetStringPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetStringPref(_, _))
      .WillByDefault(
          Invoke([](const std::string& path, const std::string& value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            Prefs()[uuid] = value;
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

void MockSetInt64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetInt64Pref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const int64_t value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
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

void MockSetUint64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetUint64Pref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const uint64_t value) {
        const std::string uuid = GetUuidForCurrentTestAndValue(path);
        Prefs()[uuid] = base::NumberToString(value);
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

void MockSetTimePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetTimePref(_, _))
      .WillByDefault(
          Invoke([](const std::string& path, const base::Time value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            Prefs()[uuid] = base::NumberToString(
                value.ToDeltaSinceWindowsEpoch().InMicroseconds());
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

void MockSetDictPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetDictPref(_, _))
      .WillByDefault(
          Invoke([](const std::string& path, base::Value::Dict value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            std::string json;
            CHECK(base::JSONWriter::Write(value, &json));
            Prefs()[uuid] = json;
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

void MockSetListPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetListPref(_, _))
      .WillByDefault(
          Invoke([](const std::string& path, base::Value::List value) {
            const std::string uuid = GetUuidForCurrentTestAndValue(path);
            std::string json;
            CHECK(base::JSONWriter::Write(value, &json));
            Prefs()[uuid] = json;
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

}  // namespace ads
