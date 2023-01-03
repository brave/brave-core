/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/campaigns_database_table.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/database/database_bind_util.h"
#include "bat/ads/internal/common/database/database_table_util.h"
#include "bat/ads/internal/common/database/database_transaction_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads::database::table {

namespace {

constexpr char kTableName[] = "campaigns";

int BindParameters(mojom::DBCommandInfo* command,
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

void MigrateToV24(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  DropTable(transaction, "campaigns");

  const std::string query =
      "CREATE TABLE campaigns "
      "(campaign_id TEXT NOT NULL PRIMARY KEY UNIQUE ON CONFLICT REPLACE, "
      "start_at_timestamp TIMESTAMP NOT NULL, "
      "end_at_timestamp TIMESTAMP NOT NULL, "
      "daily_cap INTEGER DEFAULT 0 NOT NULL, "
      "advertiser_id TEXT NOT NULL, "
      "priority INTEGER NOT NULL DEFAULT 0, "
      "ptr DOUBLE NOT NULL DEFAULT 1)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace

void Campaigns::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(transaction.get(), GetTableName());

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void Campaigns::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                               const CreativeAdList& creative_ads) {
  DCHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), creative_ads);

  transaction->commands.push_back(std::move(command));
}

std::string Campaigns::GetTableName() const {
  return kTableName;
}

void Campaigns::Migrate(mojom::DBTransactionInfo* transaction,
                        const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 24: {
      MigrateToV24(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string Campaigns::BuildInsertOrUpdateQuery(
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
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

}  // namespace ads::database::table
