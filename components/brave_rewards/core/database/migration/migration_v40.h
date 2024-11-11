/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V40_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V40_H_

namespace brave_rewards::internal::database::migration {

// Migration 40 removes the pending contributions table.
inline constexpr char v40[] = R"sql(
  DROP TABLE IF EXISTS pending_contribution;
  DROP TABLE IF EXISTS processed_publisher;
)sql";

}  // namespace brave_rewards::internal::database::migration

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V40_H_
