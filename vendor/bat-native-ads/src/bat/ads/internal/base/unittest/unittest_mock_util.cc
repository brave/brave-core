/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/unittest/unittest_mock_util.h"

#include <cstdint>
#include <ostream>
#include <utility>
#include <vector>

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
#include "bat/ads/ads.h"
#include "bat/ads/database.h"
#include "bat/ads/internal/base/unittest/unittest_file_util.h"
#include "bat/ads/internal/base/unittest/unittest_test_suite_util.h"
#include "bat/ads/internal/base/unittest/unittest_url_response_util.h"
#include "bat/ads/notification_ad_info.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace ads {

using AdEventHistoryMap = base::flat_map<std::string, std::vector<base::Time>>;
using AdEventMap = base::flat_map<std::string, AdEventHistoryMap>;

using PrefMap = base::flat_map<std::string, std::string>;

namespace {

AdEventMap& GetAdEventHistory() {
  static base::NoDestructor<AdEventMap> ad_events;
  return *ad_events;
}

PrefMap& GetPrefs() {
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

void MockLocaleHelper(const std::unique_ptr<brave_l10n::LocaleHelperMock>& mock,
                      const std::string& locale) {
  brave_l10n::LocaleHelper::GetInstance()->SetForTesting(mock.get());

  ON_CALL(*mock, GetLocale()).WillByDefault(Return(locale));
}

void MockPlatformHelper(const std::unique_ptr<PlatformHelperMock>& mock,
                        const PlatformType type) {
  PlatformHelper::GetInstance()->SetForTesting(mock.get());

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

            const std::string& uuid = GetUuidForCurrentTestSuiteAndName(id);
            const std::string& type_id = ad_type + confirmation_type;
            GetAdEventHistory()[uuid][type_id].push_back(time);
          }));
}

void MockGetAdEventHistory(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetAdEventHistory(_, _))
      .WillByDefault(Invoke(
          [](const std::string& ad_type,
             const std::string& confirmation_type) -> std::vector<base::Time> {
            CHECK(!ad_type.empty());
            CHECK(!confirmation_type.empty());

            const std::string& current_test_suite_and_name =
                GetCurrentTestSuiteAndName();

            const std::string& type_id = ad_type + confirmation_type;

            std::vector<base::Time> timestamps;

            for (const auto& ad_event : GetAdEventHistory()) {
              const std::string& uuid = ad_event.first;
              if (!base::EndsWith(uuid, current_test_suite_and_name,
                                  base::CompareCase::SENSITIVE)) {
                // Only get ad events for current test suite and name
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

                timestamps.insert(timestamps.end(),
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

        const std::string& uuid = GetUuidForCurrentTestSuiteAndName(id);
        GetAdEventHistory()[uuid] = {};
      }));
}

void MockGetBrowsingHistory(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetBrowsingHistory(_, _, _))
      .WillByDefault(Invoke([](const int max_count, const int days_ago,
                               GetBrowsingHistoryCallback callback) {
        std::vector<GURL> history;

        for (int i = 0; i < max_count; i++) {
          const std::string spec =
              base::StringPrintf("https://www.brave.com/%d", i);
          history.push_back(GURL(spec));
        }

        std::move(callback).Run(history);
      }));
}

void MockUrlRequest(const std::unique_ptr<AdsClientMock>& mock,
                    const URLEndpointMap& endpoints) {
  ON_CALL(*mock, UrlRequest(_, _))
      .WillByDefault(
          Invoke([endpoints](const mojom::UrlRequestInfoPtr& url_request,
                             UrlRequestCallback callback) {
            mojom::UrlResponseInfo url_response;

            const absl::optional<mojom::UrlResponseInfo> url_response_optional =
                GetNextUrlResponse(url_request, endpoints);
            if (url_response_optional) {
              url_response = url_response_optional.value();
            }

            callback(url_response);
          }));
}

void MockSave(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, Save(_, _, _))
      .WillByDefault(Invoke(
          [](const std::string& name, const std::string& value,
             ResultCallback callback) { callback(/* success */ true); }));
}

void MockLoad(const std::unique_ptr<AdsClientMock>& mock,
              const base::ScopedTempDir& temp_dir) {
  ON_CALL(*mock, Load(_, _))
      .WillByDefault(
          Invoke([&temp_dir](const std::string& name, LoadCallback callback) {
            base::FilePath path = temp_dir.GetPath().AppendASCII(name);
            if (!base::PathExists(path)) {
              // If path does not exist load file from the test path
              path = GetTestPath().AppendASCII(name);
            }

            std::string value;
            if (!base::ReadFileToString(path, &value)) {
              std::move(callback).Run(/* success */ false, value);
              return;
            }

            std::move(callback).Run(/* success */ true, value);
          }));
}

void MockLoadFileResource(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, LoadFileResource(_, _, _))
      .WillByDefault(Invoke([](const std::string& id, const int version,
                               LoadFileCallback callback) {
        const base::FilePath path = GetFileResourcePath().AppendASCII(id);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file));
      }));
}

void MockLoadDataResource(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, LoadDataResource(_))
      .WillByDefault(Invoke([](const std::string& name) -> std::string {
        const absl::optional<std::string>& content_optional =
            ReadFileFromDataResourcePathToString(name);
        if (!content_optional.has_value()) {
          return "";
        }

        return content_optional.value();
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
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        const std::string& value_as_string = GetPrefs()[pref_path];
        if (!value_as_string.empty()) {
          CHECK(base::StringToInt(value_as_string, &value));
        }
        return static_cast<bool>(value);
      }));
}

void MockSetBooleanPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetBooleanPref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const bool value) {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        GetPrefs()[pref_path] = base::NumberToString(static_cast<int>(value));
      }));
}

void MockGetIntegerPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetIntegerPref(_))
      .WillByDefault(Invoke([](const std::string& path) -> int {
        int value = 0;
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        const std::string& value_as_string = GetPrefs()[pref_path];
        if (!value_as_string.empty()) {
          CHECK(base::StringToInt(value_as_string, &value));
        }
        return value;
      }));
}

void MockSetIntegerPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetIntegerPref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const int value) {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        GetPrefs()[pref_path] = base::NumberToString(value);
      }));
}

void MockGetDoublePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetDoublePref(_))
      .WillByDefault(Invoke([](const std::string& path) -> double {
        double value = 0.0;
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        const std::string& value_as_string = GetPrefs()[pref_path];
        if (!value_as_string.empty()) {
          CHECK(base::StringToDouble(value_as_string, &value));
        }
        return value;
      }));
}

void MockSetDoublePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetDoublePref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const double value) {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        GetPrefs()[pref_path] = base::NumberToString(value);
      }));
}

void MockGetStringPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetStringPref(_))
      .WillByDefault(Invoke([](const std::string& path) -> std::string {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        return GetPrefs()[pref_path];
      }));
}

void MockSetStringPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetStringPref(_, _))
      .WillByDefault(Invoke([](const std::string& path,
                               const std::string& value) {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        GetPrefs()[pref_path] = value;
      }));
}

void MockGetInt64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetInt64Pref(_))
      .WillByDefault(Invoke([](const std::string& path) -> int64_t {
        int64_t value = 0;
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        const std::string& value_as_string = GetPrefs()[pref_path];
        if (!value_as_string.empty()) {
          CHECK(base::StringToInt64(value_as_string, &value));
        }
        return value;
      }));
}

void MockSetInt64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetInt64Pref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const int64_t value) {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        GetPrefs()[pref_path] = base::NumberToString(value);
      }));
}

void MockGetUint64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetUint64Pref(_))
      .WillByDefault(Invoke([](const std::string& path) -> uint64_t {
        uint64_t value = 0;
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        const std::string& value_as_string = GetPrefs()[pref_path];
        if (!value_as_string.empty()) {
          CHECK(base::StringToUint64(value_as_string, &value));
        }
        return value;
      }));
}

void MockSetUint64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetUint64Pref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const uint64_t value) {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        GetPrefs()[pref_path] = base::NumberToString(value);
      }));
}

void MockGetTimePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetTimePref(_))
      .WillByDefault(Invoke([](const std::string& path) -> base::Time {
        int64_t value = 0;
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        const std::string& value_as_string = GetPrefs()[pref_path];
        if (!value_as_string.empty()) {
          CHECK(base::StringToInt64(value_as_string, &value));
        }
        return base::Time::FromDeltaSinceWindowsEpoch(
            base::Microseconds(value));
      }));
}

void MockSetTimePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetTimePref(_, _))
      .WillByDefault(Invoke([](const std::string& path,
                               const base::Time value) {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        GetPrefs()[pref_path] = base::NumberToString(
            value.ToDeltaSinceWindowsEpoch().InMicroseconds());
      }));
}

void MockGetDictPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetDictPref(_))
      .WillByDefault(Invoke(
          [](const std::string& path) -> absl::optional<base::Value::Dict> {
            const std::string pref_path =
                GetUuidForCurrentTestSuiteAndName(path);
            const std::string& json = GetPrefs()[pref_path];
            const absl::optional<base::Value> value =
                base::JSONReader::Read(json);
            const base::Value::Dict* dict = value->GetIfDict();
            CHECK(dict);
            return dict->Clone();
          }));
}

void MockSetDictPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetDictPref(_, _))
      .WillByDefault(Invoke([](const std::string& path,
                               base::Value::Dict value) {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        std::string json;
        CHECK(base::JSONWriter::Write(value, &json));
        GetPrefs()[pref_path] = json;
      }));
}

void MockGetListPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetListPref(_))
      .WillByDefault(Invoke(
          [](const std::string& path) -> absl::optional<base::Value::List> {
            const std::string pref_path =
                GetUuidForCurrentTestSuiteAndName(path);
            const std::string& json = GetPrefs()[pref_path];
            const absl::optional<base::Value> value =
                base::JSONReader::Read(json);
            const base::Value::List* list = value->GetIfList();
            CHECK(list);
            return list->Clone();
          }));
}

void MockSetListPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetListPref(_, _))
      .WillByDefault(Invoke([](const std::string& path,
                               base::Value::List value) {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        std::string json;
        CHECK(base::JSONWriter::Write(value, &json));
        GetPrefs()[pref_path] = json;
      }));
}

void MockClearPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, ClearPref(_))
      .WillByDefault(Invoke([](const std::string& path) {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        GetPrefs().erase(pref_path);
      }));
}

void MockHasPrefPath(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, HasPrefPath(_))
      .WillByDefault(Invoke([](const std::string& path) -> bool {
        const std::string pref_path = GetUuidForCurrentTestSuiteAndName(path);
        const auto iter = GetPrefs().find(pref_path);
        if (iter == GetPrefs().end()) {
          return false;
        }

        return true;
      }));
}

}  // namespace ads
