/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions_database_util.h"

#include "base/functional/bind.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/conversions/conversions_database_table.h"

namespace ads::database {

void PurgeExpiredConversions() {
  const table::Conversions database_table;
  database_table.PurgeExpired(base::BindOnce([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to purge expired conversions");
      return;
    }

    BLOG(3, "Successfully purged expired conversions");
  }));
}

void SaveConversions(const ConversionList& conversions) {
  table::Conversions database_table;
  database_table.Save(conversions, base::BindOnce([](const bool success) {
                        if (!success) {
                          BLOG(0, "Failed to save conversions");
                          return;
                        }

                        BLOG(3, "Successfully saved conversions");
                      }));
}

}  // namespace ads::database
