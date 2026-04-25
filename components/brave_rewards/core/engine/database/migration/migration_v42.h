/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_DATABASE_MIGRATION_MIGRATION_V42_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_DATABASE_MIGRATION_MIGRATION_V42_H_

namespace brave_rewards::internal::database::migration {

// Migration 42 removes the "rewards-image" URL prefix from publisher banner
// data.
inline constexpr char v42[] = R"sql(
  UPDATE server_publisher_banner SET
    background = replace(background, 'chrome://rewards-image/', ''),
    logo = replace(logo, 'chrome://rewards-image/', '');
)sql";

}  // namespace brave_rewards::internal::database::migration

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_DATABASE_MIGRATION_MIGRATION_V42_H_
