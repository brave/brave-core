/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/predictors/loading_predictor_data_cleanup.h"

#include <algorithm>
#include <array>
#include <string_view>

#include "base/strings/strcat.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace predictors {

namespace {

// The tables holding what the loading predictor learned. Spelled out here
// instead of shared with upstream so that data written by any earlier version
// still gets cleaned up, including from tables upstream has since renamed or
// dropped.
constexpr auto kLoadingPredictorTables = std::to_array<std::string_view>({
    "resource_prefetch_predictor_host_redirect",
    "resource_prefetch_predictor_origin",
    "lcp_critical_path_predictor",
    "lcp_critical_path_predictor_initiator_origin",
});

bool HasAnyRow(sql::Database* db, std::string_view table_name) {
  if (!db->DoesTableExist(table_name)) {
    return false;
  }
  sql::Statement statement(db->GetUniqueStatement(
      base::StrCat({"SELECT 1 FROM ", table_name, " LIMIT 1"})));
  return statement.Step();
}

}  // namespace

void MaybeClearLoadingPredictorData(sql::Database* db,
                                    bool is_loading_predictor_enabled) {
  if (is_loading_predictor_enabled) {
    return;
  }
  if (std::ranges::none_of(kLoadingPredictorTables,
                           [db](std::string_view table_name) {
                             return HasAnyRow(db, table_name);
                           })) {
    return;
  }

  sql::Transaction transaction(db);
  if (!transaction.Begin()) {
    return;
  }
  for (std::string_view table_name : kLoadingPredictorTables) {
    if (!db->Execute(base::StrCat({"DROP TABLE IF EXISTS ", table_name}))) {
      return;
    }
  }
  transaction.Commit();
}

}  // namespace predictors
