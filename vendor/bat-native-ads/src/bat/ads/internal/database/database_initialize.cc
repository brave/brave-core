/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/database_initialize.h"

#include <functional>
#include <utility>

#include "base/check.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/database_migration.h"
#include "bat/ads/internal/database/database_version.h"

namespace ads {
namespace database {

Initialize::Initialize() = default;

Initialize::~Initialize() = default;

void Initialize::CreateOrOpen(ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->version = version();
  transaction->compatible_version = compatible_version();

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::INITIALIZE;

  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&Initialize::OnCreateOrOpen, this,
                                        std::placeholders::_1, callback));
}

std::string Initialize::get_last_message() const {
  return last_message_;
}

///////////////////////////////////////////////////////////////////////////////

void Initialize::OnCreateOrOpen(mojom::DBCommandResponsePtr response,
                                ResultCallback callback) {
  DCHECK(response);

  if (response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    last_message_ = "Invalid response status";
    callback(/* success */ false);
    return;
  }

  if (!response->result || response->result->get_value()->which() !=
                               mojom::DBValue::Tag::INT_VALUE) {
    last_message_ = "Invalid response result type";
    callback(/* success */ false);
    return;
  }

  const int version = response->result->get_value()->get_int_value();

  Migration migration;
  migration.FromVersion(version, callback);
}

}  // namespace database
}  // namespace ads
