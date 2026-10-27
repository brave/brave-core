/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"

namespace brave_ads::database {

void PurgeExpiredCampaignConversions() {
  const table::CampaignConversions database_table;
  database_table.PurgeExpired(base::BindOnce([](bool success) {
    if (!success) {
      return BLOG(0, "Failed to purge expired campaign conversions");
    }

    BLOG(3, "Successfully purged expired campaign conversions");
  }));
}

void SaveCampaignConversions(
    const CampaignConversionList& campaign_conversions) {
  table::CampaignConversions database_table;
  database_table.Save(campaign_conversions, base::BindOnce([](bool success) {
                        if (!success) {
                          return BLOG(0, "Failed to save campaign conversions");
                        }

                        BLOG(3, "Successfully saved campaign conversions");
                      }));
}

}  // namespace brave_ads::database
