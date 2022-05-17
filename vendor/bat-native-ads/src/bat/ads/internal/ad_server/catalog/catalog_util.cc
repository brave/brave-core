/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_server/catalog/catalog_util.h"

#include "base/time/time.h"
#include "bat/ads/internal/ad_server/catalog/catalog.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {
constexpr int kCatalogLifespanInDays = 1;
}

void ResetCatalog() {
  AdsClientHelper::Get()->ClearPref(prefs::kCatalogId);
  AdsClientHelper::Get()->ClearPref(prefs::kCatalogVersion);
  AdsClientHelper::Get()->ClearPref(prefs::kCatalogPing);
  AdsClientHelper::Get()->ClearPref(prefs::kCatalogLastUpdated);
}

std::string GetCatalogId() {
  return AdsClientHelper::Get()->GetStringPref(prefs::kCatalogId);
}

bool DoesCatalogExist() {
  return AdsClientHelper::Get()->GetIntegerPref(prefs::kCatalogVersion) > 0;
}

bool HasCatalogExpired() {
  const base::Time now = base::Time::Now();

  const double catalog_last_updated_timestamp =
      AdsClientHelper::Get()->GetDoublePref(prefs::kCatalogLastUpdated);

  const base::Time time =
      base::Time::FromDoubleT(catalog_last_updated_timestamp);

  if (now < time + base::Days(kCatalogLifespanInDays)) {
    return false;
  }

  return true;
}

}  // namespace ads
