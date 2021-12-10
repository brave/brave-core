/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/creative_ads_database_table.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {
namespace table {

namespace {

const char kTableName[] = "creative_ads";

int BindParameters(mojom::DBCommand* command,
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
    BindString(command, index++, creative_ad.target_url);

    count++;
  }

  return count;
}

CreativeAdInfo GetFromRecord(mojom::DBRecord* record) {
  DCHECK(record);

  CreativeAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(record, 0);
  creative_ad.conversion = ColumnBool(record, 1);
  creative_ad.per_day = ColumnInt(record, 2);
  creative_ad.per_week = ColumnInt(record, 3);
  creative_ad.per_month = ColumnInt(record, 4);
  creative_ad.total_max = ColumnInt(record, 5);
  creative_ad.value = ColumnDouble(record, 6);
  creative_ad.target_url = ColumnString(record, 7);

  return creative_ad;
}

CreativeAdMap GroupCreativeAdsFromResponse(
    mojom::DBCommandResponsePtr response) {
  DCHECK(response);

  CreativeAdMap creative_ads;

  for (const auto& record : response->result->get_records()) {
    const CreativeAdInfo& creative_ad = GetFromRecord(record.get());

    const auto iter = creative_ads.find(creative_ad.creative_instance_id);
    if (iter == creative_ads.end()) {
      creative_ads.insert({creative_ad.creative_instance_id, creative_ad});
      continue;
    }

    // Creative instance already exists, so append the geo targets and dayparts
    // to the existing creative ad
    iter->second.geo_targets.insert(creative_ad.geo_targets.begin(),
                                    creative_ad.geo_targets.end());

    iter->second.dayparts.insert(iter->second.dayparts.end(),
                                 creative_ad.dayparts.begin(),
                                 creative_ad.dayparts.end());
  }

  return creative_ads;
}

CreativeAdList GetCreativeAdsFromResponse(
    mojom::DBCommandResponsePtr response) {
  DCHECK(response);

  const CreativeAdMap& grouped_creative_ads =
      GroupCreativeAdsFromResponse(std::move(response));

  CreativeAdList creative_ads;
  for (const auto& grouped_creative_ad : grouped_creative_ads) {
    const CreativeAdInfo& creative_ad = grouped_creative_ad.second;
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

}  // namespace

CreativeAds::CreativeAds() = default;

CreativeAds::~CreativeAds() = default;

void CreativeAds::InsertOrUpdate(mojom::DBTransaction* transaction,
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

void CreativeAds::Delete(ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  util::Delete(transaction.get(), GetTableName());

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void CreativeAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeAdCallback callback) {
  CreativeAdInfo creative_ad;

  if (creative_instance_id.empty()) {
    callback(/* success */ false, creative_instance_id, creative_ad);
    return;
  }

  const std::string& query = base::StringPrintf(
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

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      mojom::DBCommand::RecordBindingType::BOOL_TYPE,    // conversion
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommand::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // split_test_group
      mojom::DBCommand::RecordBindingType::STRING_TYPE   // target_url
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&CreativeAds::OnGetForCreativeInstanceId, this,
                std::placeholders::_1, creative_instance_id, callback));
}

std::string CreativeAds::GetTableName() const {
  return kTableName;
}

void CreativeAds::Migrate(mojom::DBTransaction* transaction,
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

std::string CreativeAds::BuildInsertOrUpdateQuery(
    mojom::DBCommand* command,
    const CreativeAdList& creative_ads) {
  DCHECK(command);

  const int count = BindParameters(command, creative_ads);

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
      BuildBindingParameterPlaceholders(9, count).c_str());
}

void CreativeAds::OnGetForCreativeInstanceId(
    mojom::DBCommandResponsePtr response,
    const std::string& creative_instance_id,
    GetCreativeAdCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative ad");
    callback(/* success */ false, creative_instance_id, {});
    return;
  }

  const CreativeAdList& creative_ads =
      GetCreativeAdsFromResponse(std::move(response));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative ad");
    callback(/* success */ false, creative_instance_id, {});
    return;
  }

  const CreativeAdInfo& creative_ad = creative_ads.front();

  callback(/* success */ true, creative_instance_id, creative_ad);
}

void CreativeAds::MigrateToV16(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, "creative_ads");

  const std::string& query =
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

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace table
}  // namespace database
}  // namespace ads
