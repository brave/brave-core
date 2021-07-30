/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/dayparts_database_table.h"

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/daypart_frequency_cap.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {
namespace table {

namespace {
const char kTableName[] = "dayparts";
}  // namespace

Dayparts::Dayparts() = default;

Dayparts::~Dayparts() = default;

void Dayparts::InsertOrUpdate(DBTransaction* transaction,
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

void Dayparts::Delete(ResultCallback callback) {
  DBTransactionPtr transaction = DBTransaction::New();

  util::Delete(transaction.get(), get_table_name());

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string Dayparts::get_table_name() const {
  return kTableName;
}

void Dayparts::Migrate(DBTransaction* transaction, const int to_version) {
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

int Dayparts::BindParameters(DBCommand* command,
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

std::string Dayparts::BuildInsertOrUpdateQuery(
    DBCommand* command,
    const CreativeAdList& creative_ads) {
  const int count = BindParameters(command, creative_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(campaign_id, "
      "dow, "
      "start_minute, "
      "end_minute) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(4, count).c_str());
}

void Dayparts::CreateTableV15(DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
      "(campaign_id TEXT NOT NULL, "
      "dow TEXT NOT NULL, "
      "start_minute INT NOT NULL, "
      "end_minute INT NOT NULL, "
      "PRIMARY KEY (campaign_id, dow, start_minute, end_minute), "
      "UNIQUE(campaign_id, dow, start_minute, end_minute) "
      "ON CONFLICT REPLACE)",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void Dayparts::MigrateToV15(DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, get_table_name());

  CreateTableV15(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
