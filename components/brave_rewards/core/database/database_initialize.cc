/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/database/database_initialize.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace brave_rewards::internal {
namespace database {

DatabaseInitialize::DatabaseInitialize(RewardsEngineImpl& engine)
    : engine_(engine), migration_(engine) {}

DatabaseInitialize::~DatabaseInitialize() = default;

void DatabaseInitialize::Start(LegacyResultCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  transaction->version = GetCurrentVersion();
  transaction->compatible_version = GetCompatibleVersion();
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::INITIALIZE;
  transaction->commands.push_back(std::move(command));

  engine_->RunDBTransaction(
      std::move(transaction),
      std::bind(&DatabaseInitialize::OnInitialize, this, _1, callback));
}

void DatabaseInitialize::OnInitialize(mojom::DBCommandResponsePtr response,
                                      LegacyResultCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(mojom::Result::DATABASE_INIT_FAILED);
    return;
  }

  if (!response->result || !response->result->get_value()->is_int_value()) {
    BLOG(0, "DB init failed");
    callback(mojom::Result::DATABASE_INIT_FAILED);
    return;
  }

  const auto current_table_version =
      response->result->get_value()->get_int_value();
  migration_.Start(current_table_version, callback);
}

}  // namespace database
}  // namespace brave_rewards::internal
