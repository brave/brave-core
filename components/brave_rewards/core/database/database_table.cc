/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/database/database_table.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace database {

DatabaseTable::DatabaseTable(RewardsEngineImpl& engine) : engine_(engine) {}

DatabaseTable::~DatabaseTable() = default;

}  // namespace database
}  // namespace brave_rewards::internal
