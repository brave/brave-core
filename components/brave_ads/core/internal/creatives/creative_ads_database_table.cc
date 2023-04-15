/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"

#include <map>
#include <utility>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

using CreativeAdMap =
    std::map</*creative_instance_id*/ std::string, CreativeAdInfo>;

namespace {

constexpr char kTableName[] = "creative_ads";

int BindParameters(mojom::DBCommandInfo* command,
                   const CreativeAdList& creative_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.creative_instance_id);
    BindBool(command, index++, creative_ad.conversion);
    BindInt(command, index++, creative_ad.per_day);
    BindInt(command, index++, creative_ad.per_week);
    BindInt(command, index++, creative_ad.per_month);
    BindInt(command, index++, creative_ad.total_max);
    BindDouble(command, index++, creative_ad.value);
    BindString(command, index++, creative_ad.split_test_group);
    BindString(command, index++, creative_ad.target_url.spec());

    count++;
  }

  return count;
}

CreativeAdInfo GetFromRecord(mojom::DBRecordInfo* record) {
  DCHECK(record);

  CreativeAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(record, 0);
  creative_ad.conversion = ColumnBool(record, 1);
  creative_ad.per_day = ColumnInt(record, 2);
  creative_ad.per_week = ColumnInt(record, 3);
  creative_ad.per_month = ColumnInt(record, 4);
  creative_ad.total_max = ColumnInt(record, 5);
  creative_ad.value = ColumnDouble(record, 6);
  creative_ad.target_url = GURL(ColumnString(record, 7));

  return creative_ad;
}

CreativeAdMap GroupCreativeAdsFromResponse(
    mojom::DBCommandResponseInfoPtr command_response) {
  DCHECK(command_response);

  CreativeAdMap creative_ads;

  for (const auto& record : command_response->result->get_records()) {
    const CreativeAdInfo creative_ad = GetFromRecord(record.get());

    const auto iter = creative_ads.find(creative_ad.creative_instance_id);
    if (iter == creative_ads.cend()) {
      creative_ads.insert({creative_ad.creative_instance_id, creative_ad});
      continue;
    }

    // Creative instance already exists, so append new geo targets and dayparts
    // to the existing creative ad
    for (const auto& geo_target : creative_ad.geo_targets) {
      const auto geo_target_iter = iter->second.geo_targets.find(geo_target);
      if (geo_target_iter == iter->second.geo_targets.cend()) {
        iter->second.geo_targets.insert(geo_target);
      }
    }

    for (const auto& daypart : creative_ad.dayparts) {
      if (!base::Contains(iter->second.dayparts, daypart)) {
        iter->second.dayparts.push_back(daypart);
      }
    }
  }

  return creative_ads;
}

CreativeAdList GetCreativeAdsFromResponse(
    mojom::DBCommandResponseInfoPtr command_response) {
  DCHECK(command_response);

  const CreativeAdMap grouped_creative_ads =
      GroupCreativeAdsFromResponse(std::move(command_response));

  CreativeAdList creative_ads;
  for (const auto& [creative_instance_id, creative_ad] : grouped_creative_ads) {
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

void OnGetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeAdCallback callback,
    mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative ad");
    return std::move(callback).Run(/*success*/ false, creative_instance_id,
                                   /*creative_ad*/ {});
  }

  const CreativeAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(command_response));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative ad");
    return std::move(callback).Run(/*success*/ false, creative_instance_id,
                                   /*creative_ad*/ {});
  }

  const CreativeAdInfo& creative_ad = creative_ads.front();

  std::move(callback).Run(/*success*/ true, creative_instance_id, creative_ad);
}

void MigrateToV24(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  DropTable(transaction, "creative_ads");

  const std::string query =
      "CREATE TABLE creative_ads "
      "(creative_instance_id TEXT NOT NULL PRIMARY KEY UNIQUE "
      "ON CONFLICT REPLACE, "
      "conversion INTEGER NOT NULL DEFAULT 0, "
      "per_day INTEGER NOT NULL DEFAULT 0, "
      "per_week INTEGER NOT NULL DEFAULT 0, "
      "per_month INTEGER NOT NULL DEFAULT 0, "
      "total_max INTEGER NOT NULL DEFAULT 0, "
      "value DOUBLE NOT NULL DEFAULT 0, "
      "split_test_group TEXT, "
      "target_url TEXT NOT NULL)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace

void CreativeAds::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
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

void CreativeAds::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(transaction.get(), GetTableName());

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void CreativeAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeAdCallback callback) const {
  const CreativeAdInfo creative_ad;

  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success*/ false, creative_instance_id,
                                   creative_ad);
  }

  const std::string query = base::StringPrintf(
      "SELECT "
      "creative_instance_id, "
      "conversion, "
      "per_day, "
      "per_week, "
      "per_month, "
      "total_max, "
      "value, "
      "split_test_group, "
      "target_url "
      "FROM %s AS ca "
      "WHERE ca.creative_instance_id = '%s'",
      GetTableName().c_str(), creative_instance_id.c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::BOOL_TYPE,    // conversion
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // split_test_group
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE   // target_url
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetForCreativeInstanceId, creative_instance_id,
                     std::move(callback)));
}

std::string CreativeAds::GetTableName() const {
  return kTableName;
}

void CreativeAds::Migrate(mojom::DBTransactionInfo* transaction,
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

std::string CreativeAds::BuildInsertOrUpdateQuery(
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
  DCHECK(command);

  const int binded_parameters_count = BindParameters(command, creative_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creative_instance_id, "
      "conversion, "
      "per_day, "
      "per_week, "
      "per_month, "
      "total_max, "
      "value, "
      "split_test_group, "
      "target_url) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(/*parameters_count*/ 9,
                                        binded_parameters_count)
          .c_str());
}

}  // namespace brave_ads::database::table
