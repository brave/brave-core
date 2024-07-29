/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_MOCK_TEST_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_MOCK_TEST_UTIL_INTERNAL_H_

#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_mock.h"
#include "brave/components/brave_ads/core/public/database/database.h"

namespace brave_ads::test {

class TestBase;

void MockFlags();

void MockAdsClientNotifierAddObserver(AdsClientMock& mock, TestBase& test_base);

void MockShowNotificationAd(AdsClientMock& mock);
void MockCloseNotificationAd(AdsClientMock& mock);

void MockCacheAdEventForInstanceId(const AdsClientMock& mock);
void MockGetCachedAdEvents(const AdsClientMock& mock);
void MockResetAdEventCacheForInstanceId(const AdsClientMock& mock);

void MockSave(AdsClientMock& mock);
void MockLoad(AdsClientMock& mock, const base::ScopedTempDir& temp_profile_dir);

void MockLoadResourceComponent(AdsClientMock& mock,
                               const base::ScopedTempDir& temp_profile_dir);

void MockLoadDataResource(AdsClientMock& mock);

void MockRunDBTransaction(AdsClientMock& mock, Database& database);

void MockGetProfilePref(const AdsClientMock& mock);
void MockSetProfilePref(const AdsClientMock& mock, TestBase& test_base);
void MockClearProfilePref(AdsClientMock& mock);
void MockHasProfilePrefPath(const AdsClientMock& mock);

void MockGetLocalStatePref(const AdsClientMock& mock);
void MockSetLocalStatePref(const AdsClientMock& mock, TestBase& test_base);
void MockClearLocalStatePref(AdsClientMock& mock);
void MockHasLocalStatePrefPath(const AdsClientMock& mock);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_MOCK_TEST_UTIL_INTERNAL_H_
