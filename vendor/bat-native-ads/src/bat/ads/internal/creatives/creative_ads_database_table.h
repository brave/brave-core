/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_CREATIVE_ADS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_CREATIVE_ADS_DATABASE_TABLE_H_

#include <functional>
#include <string>

#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads::database::table {

using GetCreativeAdCallback =
    std::function<void(const bool success,
                       const std::string& creative_instance_id,
                       const CreativeAdInfo& ad)>;

class CreativeAds final : public TableInterface {
 public:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const CreativeAdList& creative_ads);

  void Delete(ResultCallback callback) const;

  void GetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetCreativeAdCallback callback);

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommandInfo* command,
      const CreativeAdList& creative_ads) const;

  void OnGetForCreativeInstanceId(const std::string& creative_instance_id,
                                  GetCreativeAdCallback callback,
                                  mojom::DBCommandResponseInfoPtr response);

  void MigrateToV24(mojom::DBTransactionInfo* transaction);
};

}  // namespace ads::database::table

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_CREATIVE_ADS_DATABASE_TABLE_H_
