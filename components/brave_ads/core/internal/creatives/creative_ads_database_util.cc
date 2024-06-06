/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_util.h"

#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"

namespace brave_ads::database {

void DeleteCreativeAds() {
  const table::CreativeAds database_table;
  database_table.Delete(base::BindOnce([](const bool success) {
    if (!success) {
      // TODO(https://github.com/brave/brave-browser/issues/32066):
      // Detect potential defects using `DumpWithoutCrashing`.
      SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                "Failed to delete creative ads");
      base::debug::DumpWithoutCrashing();

      return BLOG(0, "Failed to delete creative ads");
    }
  }));
}

}  // namespace brave_ads::database
