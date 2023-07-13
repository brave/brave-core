/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"

namespace brave_ads::database {

void PurgeExpiredCreativeSetConversions() {
  const table::CreativeSetConversions database_table;
  database_table.PurgeExpired(base::BindOnce([](const bool success) {
    if (!success) {
      return BLOG(0, "Failed to purge expired conversions");
    }

    BLOG(3, "Successfully purged expired conversions");
  }));
}

void SaveCreativeSetConversions(
    const CreativeSetConversionList& creative_set_conversions) {
  table::CreativeSetConversions database_table;
  database_table.Save(creative_set_conversions,
                      base::BindOnce([](const bool success) {
                        if (!success) {
                          return BLOG(0, "Failed to save conversions");
                        }

                        BLOG(3, "Successfully saved conversions");
                      }));
}

}  // namespace brave_ads::database
