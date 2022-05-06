/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/database_util.h"

#include <functional>
#include <utility>

#include "base/check.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/legacy_migration/database/database_migration.h"
#include "bat/ads/internal/legacy_migration/database/database_version.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace database {

namespace {

void OnCreateOrOpen(mojom::DBCommandResponsePtr response,
                    ResultCallback callback) {
  DCHECK(response);

  if (response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    callback(/* success */ false);
    return;
  }

  if (!response->result || !response->result->get_value()->is_int_value()) {
    callback(/* success */ false);
    return;
  }

  const int version = response->result->get_value()->get_int_value();

  Migration migration;
  migration.FromVersion(version, callback);
}

}  // namespace

void CreateOrOpen(ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->version = version();
  transaction->compatible_version = compatible_version();

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::INITIALIZE;

  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnCreateOrOpen, std::placeholders::_1, callback));
}

}  // namespace database
}  // namespace ads
