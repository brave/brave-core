/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V32_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V32_H_

namespace brave_rewards::core {
namespace database {
namespace migration {

// Migration 32 archives additional data associated with "BAP" (a deprecated
// Japan-specific presentation of BAT).
const char v32[] = R"sql(
  CREATE TABLE balance_report_info_bap AS SELECT * from balance_report_info;
  DELETE FROM balance_report_info;
)sql";

}  // namespace migration
}  // namespace database
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V32_H_
