/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V13_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V13_H_

namespace brave_rewards::core {
namespace database {
namespace migration {

const char v13[] = R"(
  ALTER TABLE promotion ADD claimed_at TIMESTAMP;
)";

}  // namespace migration
}  // namespace database
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V13_H_
