/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_MOCK_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_MOCK_TEST_UTIL_H_

#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_mock.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_mock.h"
#include "brave/components/brave_ads/core/internal/common/test/test_types.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"

namespace brave_ads::test {

using URLResponsePair =
    std::pair</*response_status_code*/ int, /*response_body*/ std::string>;
using URLResponseList = std::vector<URLResponsePair>;
using URLResponseMap = base::flat_map</*path*/ std::string, URLResponseList>;

void MockDeviceId();

void MockPlatformHelper(const PlatformHelperMock& mock, PlatformType type);

void MockBuildChannel(BuildChannelType type);

void MockIsNetworkConnectionAvailable(const AdsClientMock& mock,
                                      bool is_available);

void MockIsBrowserActive(const AdsClientMock& mock, bool is_active);
void MockIsBrowserInFullScreenMode(const AdsClientMock& mock,
                                   bool is_full_screen_mode);

void MockCanShowNotificationAds(AdsClientMock& mock, bool can_show);
void MockCanShowNotificationAdsWhileBrowserIsBackgrounded(
    const AdsClientMock& mock,
    bool can_show);

void MockGetSiteHistory(AdsClientMock& mock,
                        const SiteHistoryList& site_history);

void MockUrlResponses(AdsClientMock& mock, const URLResponseMap& url_responses);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_MOCK_TEST_UTIL_H_
