/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/catalog_frequency_cap_unittest_util.h"

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/pref_names.h"

namespace ads {

void ForceCatalogFrequencyCapPermission() {
  AdsClientHelper::Get()->SetStringPref(prefs::kCatalogId,
                                        "573c74fa-623a-4a46-adce-e688dfb7e8f5");

  AdsClientHelper::Get()->SetIntegerPref(prefs::kCatalogVersion, 1);

  AdsClientHelper::Get()->SetInt64Pref(prefs::kCatalogPing, 7200000);

  AdsClientHelper::Get()->SetDoublePref(prefs::kCatalogLastUpdated,
                                        NowAsTimestamp());
}

}  // namespace ads
