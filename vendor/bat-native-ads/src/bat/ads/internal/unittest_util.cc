/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/unittest_util.h"

#include <cstdint>

#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/ads.h"
#include "bat/ads/database.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/unittest_file_util.h"
#include "bat/ads/internal/unittest_tag_parser_util.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/pref_names.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

namespace ads {

namespace {

static base::flat_map<std::string, uint16_t> g_url_endpoint_indexes;

static base::flat_map<std::string,
                      base::flat_map<std::string, std::vector<double>>>
    g_ad_event_history;

static base::flat_map<std::string, std::string> g_prefs;

std::string GetCurrentTestSuiteAndName() {
  const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();

  return base::StringPrintf("%s.%s", test_info->test_suite_name(),
                            test_info->name());
}

std::string GetUuidForCurrentTest(const std::string& name) {
  return base::StringPrintf("%s:%s", name.c_str(),
                            GetCurrentTestSuiteAndName().c_str());
}

URLEndpointResponses GetUrlEndpointResponsesForPath(
    const URLEndpoints& endpoints,
    const std::string& path) {
  const auto iter = endpoints.find(path);
  if (iter == endpoints.end()) {
    return {};
  }

  return iter->second;
}

bool GetNextUrlEndpointResponse(const std::string& url,
                                const URLEndpoints& endpoints,
                                URLEndpointResponse* url_endpoint_response) {
  DCHECK(!url.empty()) << "Empty URL";
  DCHECK(!endpoints.empty()) << "Missing endpoints";
  DCHECK(url_endpoint_response);

  const std::string path = GURL(url).PathForRequest();

  const URLEndpointResponses url_endpoint_responses =
      GetUrlEndpointResponsesForPath(endpoints, path);
  if (url_endpoint_responses.empty()) {
    // URL endpoint responses not found for given path
    return false;
  }

  uint16_t url_endpoint_response_index = 0;

  const std::string uuid = GetUuidForCurrentTest(path);
  const auto url_endpoint_response_indexes_iter =
      g_url_endpoint_indexes.find(uuid);

  if (url_endpoint_response_indexes_iter == g_url_endpoint_indexes.end()) {
    // uuid does not exist so insert a new index set to 0 for the endpoint
    g_url_endpoint_indexes.insert({uuid, url_endpoint_response_index});
  } else {
    if (url_endpoint_response_indexes_iter->second ==
        url_endpoint_responses.size() - 1) {
      NOTREACHED() << "Missing MockUrlRequest endpoint response for " << url;
      return false;
    }

    url_endpoint_response_indexes_iter->second++;

    url_endpoint_response_index = url_endpoint_response_indexes_iter->second;
  }

  DCHECK_GE(url_endpoint_response_index, 0);
  DCHECK_LT(url_endpoint_response_index, url_endpoint_responses.size());

  *url_endpoint_response =
      url_endpoint_responses.at(url_endpoint_response_index);

  return true;
}

base::flat_map<std::string, std::string> UrlRequestHeadersToMap(
    const std::vector<std::string>& headers) {
  base::flat_map<std::string, std::string> normalized_headers;

  for (const auto& header : headers) {
    const std::vector<std::string> components = base::SplitString(
        header, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

    if (components.size() != 2) {
      NOTREACHED();
      continue;
    }

    const std::string key = components.at(0);
    const std::string value = components.at(1);

    normalized_headers[key] = value;
  }

  return normalized_headers;
}

void MockGetBooleanPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetBooleanPref(_))
      .WillByDefault(Invoke([](const std::string& path) -> bool {
        const std::string pref_path = GetUuidForCurrentTest(path);
        const std::string value = g_prefs[pref_path];

        int value_as_int;
        base::StringToInt(value, &value_as_int);
        return static_cast<bool>(value_as_int);
      }));
}

void MockSetBooleanPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetBooleanPref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const bool value) {
        const std::string pref_path = GetUuidForCurrentTest(path);
        g_prefs[pref_path] = base::NumberToString(static_cast<int>(value));
      }));
}

void MockGetIntegerPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetIntegerPref(_))
      .WillByDefault(Invoke([](const std::string& path) -> int {
        const std::string pref_path = GetUuidForCurrentTest(path);
        const std::string value = g_prefs[pref_path];

        int value_as_int;
        base::StringToInt(value, &value_as_int);
        return value_as_int;
      }));
}

void MockSetIntegerPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetIntegerPref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const int value) {
        const std::string pref_path = GetUuidForCurrentTest(path);
        g_prefs[pref_path] = base::NumberToString(value);
      }));
}

void MockGetDoublePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetDoublePref(_))
      .WillByDefault(Invoke([](const std::string& path) -> double {
        const std::string pref_path = GetUuidForCurrentTest(path);
        const std::string value = g_prefs[pref_path];

        double value_as_double;
        base::StringToDouble(value, &value_as_double);
        return value_as_double;
      }));
}

void MockSetDoublePref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetDoublePref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const double value) {
        const std::string pref_path = GetUuidForCurrentTest(path);
        g_prefs[pref_path] = base::NumberToString(value);
      }));
}

void MockGetStringPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetStringPref(_))
      .WillByDefault(Invoke([](const std::string& path) -> std::string {
        const std::string pref_path = GetUuidForCurrentTest(path);
        const std::string value = g_prefs[pref_path];
        return value;
      }));
}

void MockSetStringPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetStringPref(_, _))
      .WillByDefault(
          Invoke([](const std::string& path, const std::string& value) {
            const std::string pref_path = GetUuidForCurrentTest(path);
            g_prefs[pref_path] = value;
          }));
}

void MockGetInt64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetInt64Pref(_))
      .WillByDefault(Invoke([](const std::string& path) -> int64_t {
        const std::string pref_path = GetUuidForCurrentTest(path);
        const std::string value = g_prefs[pref_path];

        int64_t value_as_int64;
        base::StringToInt64(value, &value_as_int64);
        return value_as_int64;
      }));
}

void MockSetInt64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetInt64Pref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const int64_t value) {
        const std::string pref_path = GetUuidForCurrentTest(path);
        g_prefs[pref_path] = base::NumberToString(value);
      }));
}

void MockGetUint64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetUint64Pref(_))
      .WillByDefault(Invoke([](const std::string& path) -> uint64_t {
        const std::string pref_path = GetUuidForCurrentTest(path);
        const std::string value = g_prefs[pref_path];

        uint64_t value_as_uint64;
        base::StringToUint64(value, &value_as_uint64);
        return value_as_uint64;
      }));
}

void MockSetUint64Pref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, SetUint64Pref(_, _))
      .WillByDefault(Invoke([](const std::string& path, const uint64_t value) {
        const std::string pref_path = GetUuidForCurrentTest(path);
        g_prefs[pref_path] = base::NumberToString(value);
      }));
}

void MockClearPref(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, ClearPref(_))
      .WillByDefault(Invoke([](const std::string& path) {
        const std::string pref_path = GetUuidForCurrentTest(path);
        g_prefs.erase(pref_path);
      }));
}

void MockHasPrefPath(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, HasPrefPath(_))
      .WillByDefault(Invoke([](const std::string& path) -> bool {
        const std::string pref_path = GetUuidForCurrentTest(path);

        const auto iter = g_prefs.find(pref_path);
        if (iter == g_prefs.end()) {
          return false;
        }

        return true;
      }));
}

void MockDefaultPrefs(const std::unique_ptr<AdsClientMock>& mock) {
  mock->SetBooleanPref(prefs::kEnabled, true);

  mock->SetInt64Pref(prefs::kAdsPerHour, -1);

  mock->SetIntegerPref(prefs::kIdleTimeThreshold, 15);

  mock->SetBooleanPref(prefs::kShouldAllowConversionTracking, true);

  mock->SetBooleanPref(prefs::kShouldAllowAdsSubdivisionTargeting, false);
  mock->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "AUTO");
  mock->SetStringPref(prefs::kAutoDetectedAdsSubdivisionTargetingCode, "");

  mock->SetStringPref(prefs::kCatalogId, "");
  mock->SetIntegerPref(prefs::kCatalogVersion, 1);
  mock->SetInt64Pref(prefs::kCatalogPing, 7200000);
  mock->SetDoublePref(prefs::kCatalogLastUpdated, DistantPast().ToDoubleT());

  mock->SetInt64Pref(prefs::kIssuerPing, 0);

  mock->SetDoublePref(prefs::kNextTokenRedemptionAt,
                      DistantFuture().ToDoubleT());

  mock->SetBooleanPref(prefs::kHasMigratedConversionState, true);
  mock->SetBooleanPref(prefs::kHasMigratedRewardsState, true);
}

}  // namespace

void SetEnvironment(const mojom::Environment environment) {
  g_environment = environment;
}

void SetSysInfo(const mojom::SysInfo& sys_info) {
  SysInfo().is_uncertain_future = sys_info.is_uncertain_future;
}

void SetBuildChannel(const BuildChannelType type) {
  switch (type) {
    case BuildChannelType::kNightly: {
      BuildChannel().is_release = false;
      BuildChannel().name = "nightly";
      break;
    }

    case BuildChannelType::kBeta: {
      BuildChannel().is_release = false;
      BuildChannel().name = "beta";
      break;
    }

    case BuildChannelType::kRelease: {
      BuildChannel().is_release = true;
      BuildChannel().name = "release";
      break;
    }
  }
}

void MockLocaleHelper(const std::unique_ptr<brave_l10n::LocaleHelperMock>& mock,
                      const std::string& locale) {
  ON_CALL(*mock, GetLocale()).WillByDefault(Return(locale));
}

void MockPlatformHelper(const std::unique_ptr<PlatformHelperMock>& mock,
                        const PlatformType platform_type) {
  bool is_mobile;
  std::string platform_name;

  switch (platform_type) {
    case PlatformType::kUnknown: {
      is_mobile = false;
      platform_name = "unknown";
      break;
    }

    case PlatformType::kAndroid: {
      is_mobile = true;
      platform_name = "android";
      break;
    }

    case PlatformType::kIOS: {
      is_mobile = true;
      platform_name = "ios";
      break;
    }

    case PlatformType::kLinux: {
      is_mobile = false;
      platform_name = "linux";
      break;
    }

    case PlatformType::kMacOS: {
      is_mobile = false;
      platform_name = "macos";
      break;
    }

    case PlatformType::kWindows: {
      is_mobile = false;
      platform_name = "windows";
      break;
    }
  }

  ON_CALL(*mock, IsMobile()).WillByDefault(Return(is_mobile));

  ON_CALL(*mock, GetPlatformName()).WillByDefault(Return(platform_name));

  ON_CALL(*mock, GetPlatform()).WillByDefault(Return(platform_type));
}

void MockIsNetworkConnectionAvailable(
    const std::unique_ptr<AdsClientMock>& mock,
    const bool is_available) {
  ON_CALL(*mock, IsNetworkConnectionAvailable())
      .WillByDefault(Return(is_available));
}

void MockIsForeground(const std::unique_ptr<AdsClientMock>& mock,
                      const bool is_foreground) {
  ON_CALL(*mock, IsForeground()).WillByDefault(Return(is_foreground));
}

void MockIsFullScreen(const std::unique_ptr<AdsClientMock>& mock,
                      const bool is_full_screen) {
  ON_CALL(*mock, IsFullScreen()).WillByDefault(Return(is_full_screen));
}

void MockShouldShowNotifications(const std::unique_ptr<AdsClientMock>& mock,
                                 const bool should_show) {
  ON_CALL(*mock, ShouldShowNotifications()).WillByDefault(Return(should_show));
}

void MockShowNotification(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, ShowNotification(_))
      .WillByDefault(Invoke([](const AdNotificationInfo& ad_notification) {}));
}

void MockCloseNotification(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, CloseNotification(_))
      .WillByDefault(Invoke([](const std::string& uuid) {}));
}

void MockRecordAdEventForId(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, RecordAdEventForId(_, _, _, _))
      .WillByDefault(Invoke(
          [](const std::string& id, const std::string& ad_type,
             const std::string& confirmation_type, const double timestamp) {
            DCHECK(!id.empty());
            DCHECK(!ad_type.empty());
            DCHECK(!confirmation_type.empty());

            const std::string& uuid = GetUuidForCurrentTest(id);
            const std::string& type_id = ad_type + confirmation_type;
            g_ad_event_history[uuid][type_id].push_back(timestamp);
          }));
}

void MockGetAdEvents(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetAdEvents(_, _))
      .WillByDefault(Invoke(
          [](const std::string& ad_type,
             const std::string& confirmation_type) -> std::vector<double> {
            DCHECK(!ad_type.empty());
            DCHECK(!confirmation_type.empty());

            const std::string& current_test_suite_and_name =
                GetCurrentTestSuiteAndName();

            const std::string& type_id = ad_type + confirmation_type;

            std::vector<double> timestamps;

            for (const auto& ad_event_history : g_ad_event_history) {
              const std::string& uuid = ad_event_history.first;
              if (!base::EndsWith(uuid, current_test_suite_and_name,
                                  base::CompareCase::SENSITIVE)) {
                // Only get ad events for current test
                continue;
              }

              const base::flat_map<std::string, std::vector<double>>&
                  ad_events = ad_event_history.second;

              for (const auto& ad_event : ad_events) {
                const std::string& ad_event_type_id = ad_event.first;
                if (ad_event_type_id != type_id) {
                  continue;
                }

                const std::vector<double>& ad_event_timestamps =
                    ad_event.second;

                timestamps.insert(timestamps.end(),
                                  ad_event_timestamps.cbegin(),
                                  ad_event_timestamps.cend());
              }
            }

            return timestamps;
          }));
}

void MockResetAdEventsForId(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, ResetAdEventsForId(_))
      .WillByDefault(Invoke([](const std::string& id) {
        DCHECK(!id.empty());

        const std::string& uuid = GetUuidForCurrentTest(id);
        g_ad_event_history[uuid] = {};
      }));
}

void MockGetBrowsingHistory(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, GetBrowsingHistory(_, _, _))
      .WillByDefault(Invoke([](const int max_count, const int days_ago,
                               GetBrowsingHistoryCallback callback) {
        std::vector<std::string> history;
        for (int i = 0; i < max_count; i++) {
          const std::string entry =
              base::StringPrintf("https://www.brave.com/%d", i);
          history.push_back(entry);
        }

        callback(history);
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
              callback(/* success */ false, value);
              return;
            }

            callback(/* success */ true, value);
          }));
}

void MockLoadAdsResource(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, LoadAdsResource(_, _, _))
      .WillByDefault(Invoke(
          [](const std::string& id, const int version, LoadCallback callback) {
            base::FilePath path = GetTestPath();
            path = path.AppendASCII("resources");
            path = path.AppendASCII(id);

            std::string value;
            if (!base::ReadFileToString(path, &value)) {
              callback(/* success */ false, value);
              return;
            }

            callback(/* success */ true, value);
          }));
}

void MockLoadAdsFileResource(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, LoadAdsFileResource(_, _, _))
      .WillByDefault(Invoke([](const std::string& id, const int version,
                               LoadFileCallback callback) {
        const base::FilePath path =
            GetTestPath().AppendASCII("resources").AppendASCII(id);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        callback(std::move(file));
      }));
}

void MockLoadResourceForId(const std::unique_ptr<AdsClientMock>& mock) {
  ON_CALL(*mock, LoadResourceForId(_))
      .WillByDefault(Invoke([](const std::string& id) -> std::string {
        base::FilePath path = GetResourcesPath();
        path = path.AppendASCII(id);

        std::string value;
        base::ReadFileToString(path, &value);

        return value;
      }));
}

void MockUrlRequest(const std::unique_ptr<AdsClientMock>& mock,
                    const URLEndpoints& endpoints) {
  ON_CALL(*mock, UrlRequest(_, _))
      .WillByDefault(Invoke([&endpoints](
                                const mojom::UrlRequestPtr& url_request,
                                UrlRequestCallback callback) {
        int status_code = -1;

        std::string body;

        const base::flat_map<std::string, std::string> headers_as_map =
            UrlRequestHeadersToMap(url_request->headers);

        URLEndpointResponse url_endpoint_response;
        if (GetNextUrlEndpointResponse(url_request->url, endpoints,
                                       &url_endpoint_response)) {
          status_code = url_endpoint_response.first;
          body = url_endpoint_response.second;

          if (base::StartsWith(body, "/",
                               base::CompareCase::INSENSITIVE_ASCII)) {
            const base::StringPiece filename =
                base::TrimString(body, "//", base::TrimPositions::TRIM_LEADING);

            const base::FilePath path = GetTestPath().AppendASCII(filename);
            ASSERT_TRUE(base::ReadFileToString(path, &body));

            ParseAndReplaceTagsForText(&body);
          }
        }

        mojom::UrlResponse url_response;
        url_response.url = url_request->url;
        url_response.status_code = status_code;
        url_response.body = body;
        url_response.headers = headers_as_map;
        callback(url_response);
      }));
}

void MockRunDBTransaction(const std::unique_ptr<AdsClientMock>& mock,
                          const std::unique_ptr<Database>& database) {
  ON_CALL(*mock, RunDBTransaction(_, _))
      .WillByDefault(Invoke([&database](mojom::DBTransactionPtr transaction,
                                        RunDBTransactionCallback callback) {
        DCHECK(transaction);

        mojom::DBCommandResponsePtr response = mojom::DBCommandResponse::New();

        if (!database) {
          response->status = mojom::DBCommandResponse::Status::RESPONSE_ERROR;
        } else {
          database->RunTransaction(std::move(transaction), response.get());
        }

        callback(std::move(response));
      }));
}

void MockPrefs(const std::unique_ptr<AdsClientMock>& mock) {
  MockGetBooleanPref(mock);
  MockSetBooleanPref(mock);

  MockGetIntegerPref(mock);
  MockSetIntegerPref(mock);

  MockGetDoublePref(mock);
  MockSetDoublePref(mock);

  MockGetStringPref(mock);
  MockSetStringPref(mock);

  MockGetInt64Pref(mock);
  MockSetInt64Pref(mock);

  MockGetUint64Pref(mock);
  MockSetUint64Pref(mock);

  MockClearPref(mock);

  MockHasPrefPath(mock);

  MockDefaultPrefs(mock);
}

}  // namespace ads
