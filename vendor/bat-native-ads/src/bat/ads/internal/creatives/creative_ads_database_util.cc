/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/creative_ads_database_util.h"

#include "base/functional/bind.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/creative_ads_database_table.h"

namespace ads::database {

void DeleteCreativeAds() {
  const table::CreativeAds database_table;
  database_table.Delete(base::BindOnce([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative ads");
      return;
    }

    BLOG(3, "Successfully deleted creative ads");
  }));
}

}  // namespace ads::database
