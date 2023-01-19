/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/dayparts_database_table.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/database/database_bind_util.h"
#include "bat/ads/internal/common/database/database_table_util.h"
#include "bat/ads/internal/common/database/database_transaction_util.h"

namespace ads::database::table {

namespace {

constexpr char kTableName[] = "dayparts";

int BindParameters(mojom::DBCommandInfo* command,
                   const CreativeAdList& creative_ads) {
  DCHECK(command);

  int count = 0;
  int index = 0;

  for (const auto& creative_ad : creative_ads) {
    for (const auto& daypart : creative_ad.dayparts) {
      BindString(command, index++, creative_ad.campaign_id);
      BindString(command, index++, daypart.dow);
      BindInt(command, index++, daypart.start_minute);
      BindInt(command, index++, daypart.end_minute);

      count++;
    }
  }

  return count;
}

void MigrateToV24(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  DropTable(transaction, "dayparts");

  const std::string query =
      "CREATE TABLE dayparts "
      "(campaign_id TEXT NOT NULL, "
      "dow TEXT NOT NULL, "
      "start_minute INT NOT NULL, "
      "end_minute INT NOT NULL, "
      "PRIMARY KEY (campaign_id, dow, start_minute, end_minute), "
      "UNIQUE(campaign_id, dow, start_minute, end_minute) "
      "ON CONFLICT REPLACE)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace

void Dayparts::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
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

void Dayparts::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(transaction.get(), GetTableName());

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

std::string Dayparts::GetTableName() const {
  return kTableName;
}

void Dayparts::Migrate(mojom::DBTransactionInfo* transaction,
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

std::string Dayparts::BuildInsertOrUpdateQuery(
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
  DCHECK(command);

  const int count = BindParameters(command, creative_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(campaign_id, "
      "dow, "
      "start_minute, "
      "end_minute) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(4, count).c_str());
}

}  // namespace ads::database::table
