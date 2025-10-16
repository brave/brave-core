/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_table.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_campaign_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_daypart_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_deposit_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "campaigns";

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const std::map</*campaign_id*/ std::string,
                                  CreativeCampaignInfo>& campaigns) {
  CHECK(mojom_db_action);
  CHECK(!campaigns.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& [campaign_id, campaign] : campaigns) {
    BindColumnString(mojom_db_action, index++, campaign_id);
    BindColumnString(mojom_db_action, index++, ToString(campaign.metric_type));
    BindColumnTime(mojom_db_action, index++, campaign.start_at);
    BindColumnTime(mojom_db_action, index++, campaign.end_at);
    BindColumnInt(mojom_db_action, index++, campaign.daily_cap);
    BindColumnString(mojom_db_action, index++, campaign.advertiser_id);
    BindColumnInt(mojom_db_action, index++, campaign.priority);
    BindColumnDouble(mojom_db_action, index++, campaign.pass_through_rate);

    ++row_count;
  }

  return row_count;
}

}  // namespace

Campaigns::Campaigns() = default;

Campaigns::~Campaigns() = default;

void Campaigns::Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                       const CreativeAdList& creative_ads) {
  CHECK(mojom_db_transaction);

  if (creative_ads.empty()) {
    return;
  }

  std::map</*campaign_id*/ std::string, CreativeCampaignInfo> campaigns;
  std::map</*campaign_id*/ std::string, base::flat_set<std::string>>
      geo_targets;
  std::map</*campaign_id*/ std::string, base::flat_set<CreativeDaypartInfo>>
      dayparts;
  std::map</*creative_set_id*/ std::string, base::flat_set<std::string>>
      segments;
  std::map</*creative_instance_id*/ std::string, CreativeDepositInfo> deposits;

  for (const auto& creative_ad : creative_ads) {
    campaigns[creative_ad.campaign_id] = {
        creative_ad.metric_type,      creative_ad.start_at,
        creative_ad.end_at,           creative_ad.daily_cap,
        creative_ad.advertiser_id,    creative_ad.priority,
        creative_ad.pass_through_rate};

    geo_targets[creative_ad.campaign_id] = creative_ad.geo_targets;

    dayparts[creative_ad.campaign_id].insert(
        std::ranges::cbegin(creative_ad.dayparts),
        std::ranges::cend(creative_ad.dayparts));

    segments[creative_ad.creative_set_id].insert(creative_ad.segment);
    const std::vector<std::string> segment_hierarchy =
        base::SplitString(creative_ad.segment, "-", base::TRIM_WHITESPACE,
                          base::SPLIT_WANT_NONEMPTY);
    if (segment_hierarchy.size() > 1) {
      // Add top-level segment.
      segments[creative_ad.creative_set_id].insert(segment_hierarchy.front());
    }

    deposits[creative_ad.creative_instance_id] = {
        creative_ad.value,
        /*expire_at*/ creative_ad.end_at + base::Days(7)};
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, campaigns);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  geo_targets_database_table_.Insert(mojom_db_transaction, geo_targets);

  dayparts_database_table_.Insert(mojom_db_transaction, dayparts);

  segments_database_table_.Insert(mojom_db_transaction, segments);

  deposits_database_table_.Insert(mojom_db_transaction, deposits);
}

std::string Campaigns::GetTableName() const {
  return kTableName;
}

void Campaigns::Create(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE campaigns (
        id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        metric_type TEXT NOT NULL DEFAULT 'confirmation',
        start_at TIMESTAMP NOT NULL,
        end_at TIMESTAMP NOT NULL,
        daily_cap INTEGER DEFAULT 0 NOT NULL,
        advertiser_id TEXT NOT NULL,
        priority INTEGER NOT NULL DEFAULT 0,
        ptr DOUBLE NOT NULL DEFAULT 1
      ))");
}

void Campaigns::Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                        int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 48: {
      MigrateToV48(mojom_db_transaction);
      break;
    }

    case 52: {
      MigrateToV52(mojom_db_transaction);
      break;
    }

    default: {
      // No migration needed.
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void Campaigns::MigrateToV48(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // It is safe to recreate the table because it will be repopulated after
  // downloading the catalog post-migration. However, after this migration, we
  // should not drop the table as it will store catalog and non-catalog ad units
  // and maintain relationships with other tables.
  DropTable(mojom_db_transaction, "campaigns");
  Create(mojom_db_transaction);
}

void Campaigns::MigrateToV52(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Create a temporary table:
  //   - with a new `metric_type` column with a default value of 'confirmation',
  //     which will be corrected when the new tab page ads are updated.
  Execute(mojom_db_transaction, R"(
      CREATE TABLE campaigns_temp (
        id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        metric_type TEXT NOT NULL DEFAULT 'confirmation',
        start_at TIMESTAMP NOT NULL,
        end_at TIMESTAMP NOT NULL,
        daily_cap INTEGER DEFAULT 0 NOT NULL,
        advertiser_id TEXT NOT NULL,
        priority INTEGER NOT NULL DEFAULT 0,
        ptr DOUBLE NOT NULL DEFAULT 1
      ))");

  // Copy legacy columns to the temporary table, drop the legacy table and
  // rename the temporary table.
  const std::vector<std::string> columns = {
      "id",       "start_at", "end_at", "daily_cap", "advertiser_id",
      "priority", "ptr"};

  CopyTableColumns(mojom_db_transaction, "campaigns", "campaigns_temp", columns,
                   /*should_drop=*/true);

  RenameTable(mojom_db_transaction, "campaigns_temp", "campaigns");
}

std::string Campaigns::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const std::map</*campaign_id*/ std::string, CreativeCampaignInfo>&
        campaigns) const {
  CHECK(mojom_db_action);
  CHECK(!campaigns.empty());

  const size_t row_count = BindColumns(mojom_db_action, campaigns);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            id,
            metric_type,
            start_at,
            end_at,
            daily_cap,
            advertiser_id,
            priority,
            ptr
          ) VALUES $2)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/8, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
