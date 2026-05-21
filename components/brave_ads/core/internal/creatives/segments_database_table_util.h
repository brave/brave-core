/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_SEGMENTS_DATABASE_TABLE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_SEGMENTS_DATABASE_TABLE_UTIL_H_

#include <map>
#include <string>

#include "base/containers/flat_set.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads::database::table {

// Appends SQL actions to insert `segments` into the `segments` table.
void InsertSegments(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                    const std::map</*creative_set_id*/ std::string,
                                   base::flat_set<std::string>>& segments);

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_SEGMENTS_DATABASE_TABLE_UTIL_H_
