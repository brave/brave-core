/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_UTIL_H_

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "bat/ads/ads.h"
#include "bat/ads/database.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ads {

// A list of endpoints where the response can be inline, i.e.
//
//    {
//      "/foo/bar", {
//        {
//          net::HTTP_OK, "The quick brown fox jumps over the lazy dog"
//        }
//      }
//    }
//
// or read from a file. Filenames should begin with forward slash. i.e.
//
//    {
//      "/foo/bar", {
//        {
//          net::HTTP_OK, "/response.json"
//        }
//      }
//    }
//
// Inline responses can contain |<time:period>| tags for mocking timestamps,
// where |period| should be |now|, |distant_past|, |distant_future|, |+/-#
// seconds|, |+/-# minutes|, |+/-# hours| or |+/-# days|. i.e.
//
//    {
//      "/foo/bar", {
//        {
//          net::HTTP_OK, "An example response with a <time:+7 days> timestamp"
//        }
//      }
//    }
//
// The same endpoint can be added multiple times where responses are returned in
// the specified order, i.e.
//
//    {
//      "/foo/bar", {
//        {
//           net::HTTP_OK, "/response.json"
//        },
//        {
//           net::HTTP_CREATED, "To me there's no creativity without boundaries"
//        }
//      }
//    }

using URLEndpointResponse = std::pair<int, std::string>;
using URLEndpointResponses = std::vector<URLEndpointResponse>;
using URLEndpoints = std::map<std::string, URLEndpointResponses>;

class AdsClientMock;

base::FilePath GetTestPath();

base::Optional<std::string> ReadFileFromTestPathToString(
    const std::string& name);

base::FilePath GetResourcesPath();

base::Optional<std::string> ReadFileFromResourcePathToString(
    const std::string& name);

void SetEnvironment(const Environment environment);

void SetSysInfo(const SysInfo& sys_info);

void SetBuildChannel(const bool is_release, const std::string& name);

void MockLocaleHelper(const std::unique_ptr<brave_l10n::LocaleHelperMock>& mock,
                      const std::string& locale);

void MockPlatformHelper(const std::unique_ptr<PlatformHelperMock>& mock,
                        const PlatformType platform_type);

void MockIsNetworkConnectionAvailable(
    const std::unique_ptr<AdsClientMock>& mock,
    const bool is_available);

void MockIsForeground(const std::unique_ptr<AdsClientMock>& mock,
                      const bool is_foreground);

void MockIsFullScreen(const std::unique_ptr<AdsClientMock>& mock,
                      const bool is_full_screen);

void MockShouldShowNotifications(const std::unique_ptr<AdsClientMock>& mock,
                                 const bool should_show);

void MockShowNotification(const std::unique_ptr<AdsClientMock>& mock);

void MockCloseNotification(const std::unique_ptr<AdsClientMock>& mock);

void MockRecordAdEvent(const std::unique_ptr<AdsClientMock>& mock);

void MockGetAdEvents(const std::unique_ptr<AdsClientMock>& mock);

void MockSave(const std::unique_ptr<AdsClientMock>& mock);

void MockLoad(const std::unique_ptr<AdsClientMock>& mock);

void MockLoadAdsResource(const std::unique_ptr<AdsClientMock>& mock);

void MockLoadResourceForId(const std::unique_ptr<AdsClientMock>& mock);

void MockUrlRequest(const std::unique_ptr<AdsClientMock>& mock,
                    const URLEndpoints& endpoints);

void MockRunDBTransaction(const std::unique_ptr<AdsClientMock>& mock,
                          const std::unique_ptr<Database>& database);

void MockPrefs(const std::unique_ptr<AdsClientMock>& mock);

int64_t TimestampFromDateString(const std::string& date);
base::Time TimeFromDateString(const std::string& date);

int64_t DistantPastAsTimestamp();
base::Time DistantPast();
std::string DistantPastAsISO8601();

int64_t NowAsTimestamp();
base::Time Now();
std::string NowAsISO8601();

int64_t DistantFutureAsTimestamp();
base::Time DistantFuture();
std::string DistantFutureAsISO8601();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_UTIL_H_
