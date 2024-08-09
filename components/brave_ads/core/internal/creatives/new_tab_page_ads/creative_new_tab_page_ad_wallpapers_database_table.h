/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_WALLPAPERS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_WALLPAPERS_DATABASE_TABLE_H_

#include <string>

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

namespace brave_ads::database::table {

class CreativeNewTabPageAdWallpapers final : public TableInterface {
 public:
  void Insert(mojom::DBTransactionInfo* mojom_transaction,
              const CreativeNewTabPageAdList& creative_ads);

  void Delete(ResultCallback callback) const;

  std::string GetTableName() const override;

  void Create(mojom::DBTransactionInfo* mojom_transaction) override;
  void Migrate(mojom::DBTransactionInfo* mojom_transaction,
               int to_version) override;

 private:
  void MigrateToV43(mojom::DBTransactionInfo* mojom_transaction);

  std::string BuildInsertSql(
      mojom::DBStatementInfo* mojom_statement,
      const CreativeNewTabPageAdList& creative_ads) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_WALLPAPERS_DATABASE_TABLE_H_
