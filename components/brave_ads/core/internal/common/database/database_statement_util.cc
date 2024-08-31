/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "sql/statement.h"

namespace brave_ads::database {

std::string TimeToSqlValueAsString(base::Time time) {
  return base::NumberToString(sql::Statement::TimeToSqlValue(time));
}

}  // namespace brave_ads::database
