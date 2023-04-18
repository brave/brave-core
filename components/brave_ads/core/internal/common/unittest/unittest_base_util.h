/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_BASE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_BASE_UTIL_H_

#include <memory>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_ads/core/database.h"
#include "brave/components/brave_ads/core/internal/ads_client_mock.h"

namespace brave_ads {

using PrefMap = base::flat_map</*uuid*/ std::string, /*value*/ std::string>;

PrefMap& Prefs();

void MockShowNotificationAd(const std::unique_ptr<AdsClientMock>& mock);
void MockCloseNotificationAd(const std::unique_ptr<AdsClientMock>& mock);

void MockRecordAdEventForId(const std::unique_ptr<AdsClientMock>& mock);
void MockGetAdEventHistory(const std::unique_ptr<AdsClientMock>& mock);
void MockResetAdEventHistoryForId(const std::unique_ptr<AdsClientMock>& mock);

void MockSave(const std::unique_ptr<AdsClientMock>& mock);
void MockLoad(const std::unique_ptr<AdsClientMock>& mock,
              const base::ScopedTempDir& temp_dir);
void MockLoadFileResource(const std::unique_ptr<AdsClientMock>& mock,
                          const base::ScopedTempDir& temp_dir);
void MockLoadDataResource(const std::unique_ptr<AdsClientMock>& mock);

void MockRunDBTransaction(const std::unique_ptr<AdsClientMock>& mock,
                          const std::unique_ptr<Database>& database);

void MockGetBooleanPref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetIntegerPref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetDoublePref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetStringPref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetInt64Pref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetUint64Pref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetTimePref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetDictPref(const std::unique_ptr<AdsClientMock>& mock);
void MockGetListPref(const std::unique_ptr<AdsClientMock>& mock);
void MockClearPref(const std::unique_ptr<AdsClientMock>& mock);
void MockHasPrefPath(const std::unique_ptr<AdsClientMock>& mock);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_BASE_UTIL_H_
