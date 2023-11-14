/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CATALOG_DAYPART_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CATALOG_DAYPART_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"

namespace brave_ads {

struct CatalogDaypartInfo final {
  bool operator==(const CatalogDaypartInfo&) const = default;

  std::string days_of_week =
      "0123456";  // Sunday=0, Monday=1, Tuesday=2, Wednesday=3, Thursday=4,
                  // Friday=5 and Saturday=6
  int start_minute = 0;                                             // 00:00
  int end_minute = (base::Days(1) - base::Minutes(1)).InMinutes();  // 23:59
};

using CatalogDaypartList = std::vector<CatalogDaypartInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CATALOG_DAYPART_INFO_H_
