/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_WALLPAPERS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_WALLPAPERS_DATABASE_TABLE_H_

#include <string>

#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads::database::table {

class CreativeNewTabPageAdWallpapers final : public TableInterface {
 public:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const CreativeNewTabPageAdList& creative_ads);

  void Delete(ResultCallback callback) const;

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommandInfo* command,
      const CreativeNewTabPageAdList& creative_ads) const;
};

}  // namespace ads::database::table

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_WALLPAPERS_DATABASE_TABLE_H_
