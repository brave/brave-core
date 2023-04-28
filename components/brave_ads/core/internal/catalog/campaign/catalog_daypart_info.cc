/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/catalog_daypart_info.h"

namespace brave_ads {

bool operator==(const CatalogDaypartInfo& lhs, const CatalogDaypartInfo& rhs) {
  return lhs.days_of_week == rhs.days_of_week &&
         lhs.start_minute == rhs.start_minute &&
         lhs.end_minute == rhs.end_minute;
}

bool operator!=(const CatalogDaypartInfo& lhs, const CatalogDaypartInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
