/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SQL_SQL_NAME_VARIANTS_H_
#define BRAVE_CHROMIUM_SRC_SQL_SQL_NAME_VARIANTS_H_

#define IsValidDatabaseTag IsValidDatabaseTag_ChomiumImpl
#include "../gen/sql/sql_name_variants.h"  // IWYU pragma: export
#undef IsValidDatabaseTag

namespace sql_metrics {

// In Chromium this check ensures the histograms.xml sync, but we don't care
// about this because it's for server-side synchronization.
constexpr bool IsValidDatabaseTag(std::string_view s) {
  return true;
}

}  // namespace sql_metrics

#endif  // BRAVE_CHROMIUM_SRC_SQL_SQL_NAME_VARIANTS_H_
