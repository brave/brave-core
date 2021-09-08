/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/geo_targets_database_table.h"

#include <algorithm>
#include <functional>
#include <utility>

#include "base/check.h"
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
const char kTableName[] = "geo_targets";
}  // namespace

GeoTargets::GeoTargets() = default;

GeoTargets::~GeoTargets() = default;

void GeoTargets::InsertOrUpdate(mojom::DBTransaction* transaction,
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

void GeoTargets::Delete(ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  util::Delete(transaction.get(), get_table_name());

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string GeoTargets::get_table_name() const {
  return kTableName;
}

void GeoTargets::Migrate(mojom::DBTransaction* transaction,
                         const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 16: {
      MigrateToV16(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

int GeoTargets::BindParameters(mojom::DBCommand* command,
                               const CreativeAdList& creative_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    for (const auto& geo_target : creative_ad.geo_targets) {
      BindString(command, index++, creative_ad.campaign_id);
      BindString(command, index++, geo_target);

      count++;
    }
  }

  return count;
}

std::string GeoTargets::BuildInsertOrUpdateQuery(
    mojom::DBCommand* command,
    const CreativeAdList& creative_ads) {
  const int count = BindParameters(command, creative_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(campaign_id, "
      "geo_target) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(2, count).c_str());
}

void GeoTargets::CreateTableV16(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
      "(campaign_id TEXT NOT NULL, "
      "geo_target TEXT NOT NULL, "
      "PRIMARY KEY (campaign_id, geo_target), "
      "UNIQUE(campaign_id, geo_target) ON CONFLICT REPLACE)",
      get_table_name().c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void GeoTargets::MigrateToV16(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, get_table_name());

  CreateTableV16(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
