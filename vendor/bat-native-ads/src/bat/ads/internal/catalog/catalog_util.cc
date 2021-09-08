/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_util.h"

#include <cstdint>

#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/catalog/catalog.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {
const int kCatalogLifespanInDays = 1;
}

bool DoesCatalogExist() {
  return AdsClientHelper::Get()->GetIntegerPref(prefs::kCatalogVersion) > 0;
}

bool HasCatalogExpired() {
  const base::Time now = base::Time::Now();

  const int64_t catalog_last_updated =
      AdsClientHelper::Get()->GetInt64Pref(prefs::kCatalogLastUpdated);

  const base::Time time = base::Time::FromDoubleT(catalog_last_updated);

  if (now < time + base::TimeDelta::FromDays(kCatalogLifespanInDays)) {
    return false;
  }

  return true;
}

}  // namespace ads
