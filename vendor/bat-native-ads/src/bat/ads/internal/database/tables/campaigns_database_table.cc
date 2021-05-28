/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/campaigns_database_table.h"

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {
namespace table {

namespace {
const char kTableName[] = "campaigns";
}  // namespace

Campaigns::Campaigns() = default;

Campaigns::~Campaigns() = default;

void Campaigns::Delete(ResultCallback callback) {
  DBTransactionPtr transaction = DBTransaction::New();

  util::Delete(transaction.get(), get_table_name());

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void Campaigns::InsertOrUpdate(DBTransaction* transaction,
                               const CreativeAdList& creative_ads) {
  DCHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), creative_ads);

  transaction->commands.push_back(std::move(command));
}

std::string Campaigns::get_table_name() const {
  return kTableName;
}

void Campaigns::Migrate(DBTransaction* transaction, const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 15: {
      MigrateToV15(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

int Campaigns::BindParameters(DBCommand* command,
                              const CreativeAdList& creative_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.campaign_id);
    BindInt64(command, index++, creative_ad.start_at_timestamp);
    BindInt64(command, index++, creative_ad.end_at_timestamp);
    BindInt(command, index++, creative_ad.daily_cap);
    BindString(command, index++, creative_ad.advertiser_id);
    BindInt(command, index++, creative_ad.priority);
    BindDouble(command, index++, creative_ad.ptr);

    count++;
  }

  return count;
}

std::string Campaigns::BuildInsertOrUpdateQuery(
    DBCommand* command,
    const CreativeAdList& creative_ads) {
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
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(7, count).c_str());
}

void Campaigns::CreateTableV15(DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
      "(campaign_id TEXT NOT NULL PRIMARY KEY UNIQUE ON CONFLICT REPLACE, "
      "start_at_timestamp TIMESTAMP NOT NULL, "
      "end_at_timestamp TIMESTAMP NOT NULL, "
      "daily_cap INTEGER DEFAULT 0 NOT NULL, "
      "advertiser_id TEXT NOT NULL, "
      "priority INTEGER NOT NULL DEFAULT 0, "
      "ptr DOUBLE NOT NULL DEFAULT 1)",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void Campaigns::MigrateToV15(DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, get_table_name());

  CreateTableV15(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
