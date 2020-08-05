/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/database_initialize.h"

#include <functional>
#include <utility>

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/database/database_migration.h"
#include "bat/ads/internal/database/database_version.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {

using std::placeholders::_1;

Initialize::Initialize(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

Initialize::~Initialize() = default;

void Initialize::CreateOrOpen(
    ResultCallback callback) {
  DBTransactionPtr transaction = DBTransaction::New();
  transaction->version = version();
  transaction->compatible_version = compatible_version();

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::INITIALIZE;

  transaction->commands.push_back(std::move(command));

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&Initialize::OnCreateOrOpen, this, _1, callback));
}

std::string Initialize::get_last_message() const {
  return last_message_;
}

///////////////////////////////////////////////////////////////////////////////

void Initialize::OnCreateOrOpen(
    DBCommandResponsePtr response,
    ResultCallback callback) {
  DCHECK(response);

  if (response->status != DBCommandResponse::Status::RESPONSE_OK) {
    last_message_ = "Invalid response status";
    callback(Result::FAILED);
    return;
  }

  if (!response->result || response->result->get_value()->which() !=
      DBValue::Tag::INT_VALUE) {
    last_message_ = "Invalid response result type";
    callback(Result::FAILED);
    return;
  }

  const int version = response->result->get_value()->get_int_value();

  Migration migration(ads_);
  migration.FromVersion(version, callback);
}

}  // namespace database
}  // namespace ads
