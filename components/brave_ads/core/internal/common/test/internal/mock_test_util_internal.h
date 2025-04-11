/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_MOCK_TEST_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_MOCK_TEST_UTIL_INTERNAL_H_

#include "base/files/file_path.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_mock.h"

namespace brave_ads::test {

class TestBase;

void MockFlags();

void MockContentSettings();

void MockAdsClientNotifierAddObserver(AdsClientMock& ads_client_mock,
                                      TestBase& test_base);
void MockNotifyPendingObservers(AdsClientMock& ads_client_mock,
                                TestBase& test_base);

void MockShowNotificationAd(AdsClientMock& ads_client_mock);
void MockCloseNotificationAd(AdsClientMock& ads_client_mock);

void MockSave(AdsClientMock& ads_client_mock);
void MockLoad(AdsClientMock& ads_client_mock,
              const base::FilePath& profile_path);

void MockLoadResourceComponent(AdsClientMock& ads_client_mock,
                               const base::FilePath& profile_path);

void MockFindProfilePref(const AdsClientMock& ads_client_mock);
void MockGetProfilePref(const AdsClientMock& ads_client_mock);
void MockSetProfilePref(const AdsClientMock& ads_client_mock,
                        TestBase& test_base);
void MockClearProfilePref(AdsClientMock& ads_client_mock);
void MockHasProfilePrefPath(const AdsClientMock& ads_client_mock);

void MockFindLocalStatePref(const AdsClientMock& ads_client_mock);
void MockGetLocalStatePref(const AdsClientMock& ads_client_mock);
void MockSetLocalStatePref(const AdsClientMock& ads_client_mock,
                           TestBase& test_base);
void MockClearLocalStatePref(AdsClientMock& ads_client_mock);
void MockHasLocalStatePrefPath(const AdsClientMock& ads_client_mock);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_MOCK_TEST_UTIL_INTERNAL_H_
