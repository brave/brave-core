/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CAMPAIGNS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CAMPAIGNS_DATABASE_TABLE_H_

#include <map>
#include <string>

#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/dayparts_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/geo_targets_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/segments_database_table.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct CreativeCampaignInfo;

namespace database::table {

class Campaigns final : public TableInterface {
 public:
  Campaigns();

  Campaigns(const Campaigns&) = delete;
  Campaigns& operator=(const Campaigns&) = delete;

  ~Campaigns() override;

  void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
              const CreativeAdList& creative_ads);

  std::string GetTableName() const override;

  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  void MigrateToV48(const mojom::DBTransactionInfoPtr& mojom_db_transaction);
  void MigrateToV52(const mojom::DBTransactionInfoPtr& mojom_db_transaction);

  std::string BuildInsertSql(
      const mojom::DBActionInfoPtr& mojom_db_action,
      const std::map</*campaign_id*/ std::string, CreativeCampaignInfo>&
          campaigns) const;

  GeoTargets geo_targets_database_table_;
  Dayparts dayparts_database_table_;
  Segments segments_database_table_;
  Deposits deposits_database_table_;
};

}  // namespace database::table

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CAMPAIGNS_DATABASE_TABLE_H_
