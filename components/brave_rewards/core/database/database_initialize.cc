/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/database/database_initialize.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

DatabaseInitialize::DatabaseInitialize(RewardsEngine& engine)
    : engine_(engine), migration_(engine) {}

DatabaseInitialize::~DatabaseInitialize() = default;

void DatabaseInitialize::Start(ResultCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  transaction->version = GetCurrentVersion();
  transaction->compatible_version = GetCompatibleVersion();
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::INITIALIZE;
  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseInitialize::OnInitialize, base::Unretained(this),
                     std::move(callback)));
}

void DatabaseInitialize::OnInitialize(ResultCallback callback,
                                      mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    return std::move(callback).Run(mojom::Result::DATABASE_INIT_FAILED);
  }

  if (!response->result || !response->result->get_value()->is_int_value()) {
    engine_->LogError(FROM_HERE) << "DB init failed";
    return std::move(callback).Run(mojom::Result::DATABASE_INIT_FAILED);
  }

  const auto current_table_version =
      response->result->get_value()->get_int_value();
  migration_.Start(current_table_version, std::move(callback));
}

}  // namespace brave_rewards::internal::database
