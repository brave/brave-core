/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_table.h"

namespace brave_ads::database {

void DeleteCampaigns() {
  const table::Campaigns database_table;
  database_table.Delete(base::BindOnce([](bool success) {
    if (!success) {
      BLOG(0, "Failed to delete campaigns");
    }
  }));
}

}  // namespace brave_ads::database
