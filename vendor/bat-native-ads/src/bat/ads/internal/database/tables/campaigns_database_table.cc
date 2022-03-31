/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/campaigns_database_table.h"

#include <functional>
#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"

namespace ads {
namespace database {
namespace table {

namespace {

constexpr char kTableName[] = "campaigns";

int BindParameters(mojom::DBCommand* command,
                   const CreativeAdList& creative_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.campaign_id);
    BindDouble(command, index++, creative_ad.start_at.ToDoubleT());
    BindDouble(command, index++, creative_ad.end_at.ToDoubleT());
    BindInt(command, index++, creative_ad.daily_cap);
    BindString(command, index++, creative_ad.advertiser_id);
    BindInt(command, index++, creative_ad.priority);
    BindDouble(command, index++, creative_ad.ptr);

    count++;
  }

  return count;
}

}  // namespace

Campaigns::Campaigns() = default;

Campaigns::~Campaigns() = default;

void Campaigns::Delete(ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  util::Delete(transaction.get(), GetTableName());

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void Campaigns::InsertOrUpdate(mojom::DBTransaction* transaction,
                               const CreativeAdList& creative_ads) {
  DCHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), creative_ads);

  transaction->commands.push_back(std::move(command));
}

std::string Campaigns::GetTableName() const {
  return kTableName;
}

void Campaigns::Migrate(mojom::DBTransaction* transaction,
                        const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 19: {
      MigrateToV19(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string Campaigns::BuildInsertOrUpdateQuery(
    mojom::DBCommand* command,
    const CreativeAdList& creative_ads) {
  DCHECK(command);

  const int count = BindParameters(command, creative_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(campaign_id, "
      "start_at_timestamp, "
      "end_at_timestamp, "
      "daily_cap, "
      "advertiser_id, "
      "priority, "
      "ptr) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(7, count).c_str());
}

void Campaigns::MigrateToV19(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, GetTableName());

  const std::string& query =
      "CREATE TABLE campaigns "
      "(campaign_id TEXT NOT NULL PRIMARY KEY UNIQUE ON CONFLICT REPLACE, "
      "start_at_timestamp TIMESTAMP NOT NULL, "
      "end_at_timestamp TIMESTAMP NOT NULL, "
      "daily_cap INTEGER DEFAULT 0 NOT NULL, "
      "advertiser_id TEXT NOT NULL, "
      "priority INTEGER NOT NULL DEFAULT 0, "
      "ptr DOUBLE NOT NULL DEFAULT 1)";

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace table
}  // namespace database
}  // namespace ads
