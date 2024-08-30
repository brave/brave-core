/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"

namespace brave_ads::database {

void DeleteCreativeInlineContentAds() {
  const table::CreativeInlineContentAds database_table;
  database_table.Delete(base::BindOnce([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative inline content ads");
    }
  }));
}

void SaveCreativeInlineContentAds(
    const CreativeInlineContentAdList& creative_ads) {
  table::CreativeInlineContentAds database_table;
  database_table.Save(creative_ads, base::BindOnce([](const bool success) {
                        if (!success) {
                          return BLOG(
                              0, "Failed to save creative inline content ads");
                        }

                        BLOG(3,
                             "Successfully saved creative inline content ads");
                      }));
}

}  // namespace brave_ads::database
