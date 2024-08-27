/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_ROW_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_ROW_UTIL_H_

#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "sql/statement.h"

namespace brave_ads::database {

[[nodiscard]] mojom::DBRowInfoPtr CreateRow(
    sql::Statement* statement,
    const std::vector<mojom::DBBindColumnType>& mojom_db_bind_column_types);

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_ROW_UTIL_H_
