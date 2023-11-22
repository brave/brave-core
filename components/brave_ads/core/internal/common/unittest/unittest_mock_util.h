/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_MOCK_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_MOCK_UTIL_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/client/ads_client_mock.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper_mock.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_build_channel_types.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_alias.h"

class GURL;

namespace brave_ads {

void MockDeviceId();

void MockBuildChannel(BuildChannelType type);

void MockPlatformHelper(const PlatformHelperMock& mock, PlatformType type);

void MockIsNetworkConnectionAvailable(const AdsClientMock& mock,
                                      bool is_available);

void MockIsBrowserActive(const AdsClientMock& mock, bool is_active);
void MockIsBrowserInFullScreenMode(const AdsClientMock& mock,
                                   bool is_full_screen_mode);

void MockCanShowNotificationAds(AdsClientMock& mock, bool can_show);
void MockCanShowNotificationAdsWhileBrowserIsBackgrounded(
    const AdsClientMock& mock,
    bool can_show);

void MockGetBrowsingHistory(AdsClientMock& mock,
                            const std::vector<GURL>& history);

void MockUrlResponses(AdsClientMock& mock, const URLResponseMap& url_responses);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_MOCK_UTIL_H_
