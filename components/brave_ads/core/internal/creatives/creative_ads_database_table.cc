/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"

#include <cstddef>
#include <map>
#include <utility>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads::database::table {

using CreativeAdMap =
    std::map</*creative_ad_uuid*/ std::string, CreativeAdInfo>;

namespace {

constexpr char kTableName[] = "creative_ads";

void BindRecords(mojom::DBCommandInfo* const command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_day
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_week
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // per_month
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,     // total_max
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // split_test_group
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE   // target_url
  };
}

size_t BindParameters(mojom::DBCommandInfo* command,
                      const CreativeAdList& creative_ads) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.creative_instance_id);
    BindString(command, index++, creative_ad.creative_set_id);
    BindInt(command, index++, creative_ad.per_day);
    BindInt(command, index++, creative_ad.per_week);
    BindInt(command, index++, creative_ad.per_month);
    BindInt(command, index++, creative_ad.total_max);
    BindDouble(command, index++, creative_ad.value);
    BindString(command, index++, creative_ad.split_test_group);
    BindString(command, index++, creative_ad.target_url.spec());

    ++count;
  }

  return count;
}

CreativeAdInfo GetFromRecord(mojom::DBRecordInfo* const record) {
  CHECK(record);

  CreativeAdInfo creative_ad;

  creative_ad.creative_instance_id = ColumnString(record, 0);
  creative_ad.creative_set_id = ColumnString(record, 1);
  creative_ad.per_day = ColumnInt(record, 2);
  creative_ad.per_week = ColumnInt(record, 3);
  creative_ad.per_month = ColumnInt(record, 4);
  creative_ad.total_max = ColumnInt(record, 5);
  creative_ad.value = ColumnDouble(record, 6);
  creative_ad.target_url = GURL(ColumnString(record, 7));

  return creative_ad;
}

CreativeAdList GetCreativeAdsFromResponse(
    mojom::DBCommandResponseInfoPtr command_response) {
  CHECK(command_response);
  CHECK(command_response->result);

  CreativeAdMap creative_ads;

  for (const auto& record : command_response->result->get_records()) {
    const CreativeAdInfo creative_ad = GetFromRecord(&*record);

    const std::string uuid =
        base::StrCat({creative_ad.creative_instance_id, creative_ad.segment});
    const auto iter = creative_ads.find(uuid);
    if (iter == creative_ads.cend()) {
      creative_ads.insert({uuid, creative_ad});
      continue;
    }

    for (const auto& geo_target : creative_ad.geo_targets) {
      if (!base::Contains(iter->second.geo_targets, geo_target)) {
        iter->second.geo_targets.insert(geo_target);
      }
    }

    for (const auto& daypart : creative_ad.dayparts) {
      if (!base::Contains(iter->second.dayparts, daypart)) {
        iter->second.dayparts.push_back(daypart);
      }
    }
  }

  CreativeAdList normalized_creative_ads;
  for (const auto& [_, creative_ad] : creative_ads) {
    normalized_creative_ads.push_back(creative_ad);
  }

  return normalized_creative_ads;
}

void GetForCreativeInstanceIdCallback(
    const std::string& creative_instance_id,
    GetCreativeAdCallback callback,
    mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative ad");

    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  const CreativeAdList creative_ads =
      GetCreativeAdsFromResponse(std::move(command_response));

  if (creative_ads.size() != 1) {
    BLOG(0, "Failed to get creative ad");

    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*creative_ad=*/{});
  }

  const CreativeAdInfo& creative_ad = creative_ads.front();

  std::move(callback).Run(/*success=*/true, creative_instance_id, creative_ad);
}

}  // namespace

void CreativeAds::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
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

void CreativeAds::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(&*transaction, GetTableName());

  RunTransaction(std::move(transaction), std::move(callback));
}

void CreativeAds::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetCreativeAdCallback callback) const {
  const CreativeAdInfo creative_ad;

  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   creative_ad);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_instance_id,
            creative_set_id,
            per_day,
            per_week,
            per_month,
            total_max,
            value,
            split_test_group,
            target_url
          FROM
            $1
          WHERE
            creative_instance_id = '$2';)",
      {GetTableName(), creative_instance_id}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetForCreativeInstanceIdCallback,
                                  creative_instance_id, std::move(callback)));
}

std::string CreativeAds::GetTableName() const {
  return kTableName;
}

void CreativeAds::Create(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE creative_ads (
            creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            creative_set_id TEXT NOT NULL,
            per_day INTEGER NOT NULL DEFAULT 0,
            per_week INTEGER NOT NULL DEFAULT 0,
            per_month INTEGER NOT NULL DEFAULT 0,
            total_max INTEGER NOT NULL DEFAULT 0,
            value DOUBLE NOT NULL DEFAULT 0,
            split_test_group TEXT,
            target_url TEXT NOT NULL
          );)";
  transaction->commands.push_back(std::move(command));
}

void CreativeAds::Migrate(mojom::DBTransactionInfo* transaction,
                          const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 35: {
      MigrateToV35(transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void CreativeAds::MigrateToV35(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(transaction, GetTableName());
  Create(transaction);
}

std::string CreativeAds::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            creative_set_id,
            per_day,
            per_week,
            per_month,
            total_max,
            value,
            split_test_group,
            target_url
          ) VALUES $2;)",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count=*/9, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
