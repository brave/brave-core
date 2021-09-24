/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_UTIL_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace absl {
template <typename T>
class optional;
}  // namespace absl

namespace base {
class Time;
}  // namespace base

namespace brave_l10n {
class LocaleHelperMock;
}  // namespace brave_l10n

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
class Database;

void SetEnvironment(const mojom::Environment environment);

void SetSysInfo(const mojom::SysInfo& sys_info);

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
void MockResetAdEvents(const std::unique_ptr<AdsClientMock>& mock);

void MockGetBrowsingHistory(const std::unique_ptr<AdsClientMock>& mock);

void MockSave(const std::unique_ptr<AdsClientMock>& mock);
void MockLoad(const std::unique_ptr<AdsClientMock>& mock,
              const base::ScopedTempDir& temp_dir);

void MockLoadAdsResource(const std::unique_ptr<AdsClientMock>& mock);
void MockLoadResourceForId(const std::unique_ptr<AdsClientMock>& mock);

void MockUrlRequest(const std::unique_ptr<AdsClientMock>& mock,
                    const URLEndpoints& endpoints);

void MockRunDBTransaction(const std::unique_ptr<AdsClientMock>& mock,
                          const std::unique_ptr<Database>& database);

void MockPrefs(const std::unique_ptr<AdsClientMock>& mock);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_UTIL_H_
