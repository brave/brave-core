/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V14_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V14_H_

namespace brave_rewards::core {
namespace database {
namespace migration {

const char v14[] = R"(
  UPDATE promotion SET approximate_value = (SELECT (suggestions * 0.25)
  FROM promotion as ps WHERE ps.promotion_id = promotion.promotion_id);

  UPDATE unblinded_tokens SET value = 0.25;
)";

}  // namespace migration
}  // namespace database
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_MIGRATION_MIGRATION_V14_H_
