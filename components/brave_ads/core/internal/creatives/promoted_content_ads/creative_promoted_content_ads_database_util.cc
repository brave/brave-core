/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"

namespace brave_ads::database {

void DeleteCreativePromotedContentAds() {
  const table::CreativePromotedContentAds database_table;
  database_table.Delete(base::BindOnce([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative promoted content ads");
    }
  }));
}

void SaveCreativePromotedContentAds(
    const CreativePromotedContentAdList& creative_ads) {
  table::CreativePromotedContentAds database_table;
  database_table.Save(
      creative_ads, base::BindOnce([](const bool success) {
        if (!success) {
          return BLOG(0, "Failed to save creative promoted content ads");
        }

        BLOG(3, "Successfully saved creative promoted content ads");
      }));
}

}  // namespace brave_ads::database
