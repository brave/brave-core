/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/geo_targets_database_table.h"

#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "geo_targets";

size_t BindParameters(mojom::DBCommandInfo* command,
                      const CreativeAdList& creative_ads) {
  CHECK(command);

  size_t count = 0;

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

void MigrateToV24(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  DropTable(transaction, "geo_targets");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE geo_targets (campaign_id TEXT NOT NULL, geo_target TEXT "
      "NOT NULL, PRIMARY KEY (campaign_id, geo_target), UNIQUE(campaign_id, "
      "geo_target) ON CONFLICT REPLACE);";
  transaction->commands.push_back(std::move(command));
}

}  // namespace

void GeoTargets::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                                const CreativeAdList& creative_ads) {
  CHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, creative_ads);
  transaction->commands.push_back(std::move(command));
}

void GeoTargets::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(&*transaction, GetTableName());

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string GeoTargets::GetTableName() const {
  return kTableName;
}

void GeoTargets::Create(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE geo_targets (campaign_id TEXT NOT NULL, geo_target TEXT "
      "NOT NULL, PRIMARY KEY (campaign_id, geo_target), UNIQUE(campaign_id, "
      "geo_target) ON CONFLICT REPLACE);";
  transaction->commands.push_back(std::move(command));
}

void GeoTargets::Migrate(mojom::DBTransactionInfo* transaction,
                         const int to_version) {
  CHECK(transaction);

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

std::string GeoTargets::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, creative_ads);

  return base::ReplaceStringPlaceholders(
      "INSERT OR REPLACE INTO $1 (campaign_id, geo_target) VALUES $2;",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count*/ 2, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
