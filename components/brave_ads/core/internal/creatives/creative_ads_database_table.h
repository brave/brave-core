/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_ADS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_ADS_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

namespace brave_ads::database::table {

using GetCreativeAdCallback =
    base::OnceCallback<void(bool success,
                            const std::string& creative_instance_id,
                            const CreativeAdInfo& creative_ad)>;

class CreativeAds final : public TableInterface {
 public:
  void Insert(mojom::DBTransactionInfo* mojom_db_transaction,
              const CreativeAdList& creative_ads);

  void Delete(ResultCallback callback) const;

  void GetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetCreativeAdCallback callback) const;

  std::string GetTableName() const override;

  void Create(mojom::DBTransactionInfo* mojom_db_transaction) override;
  void Migrate(mojom::DBTransactionInfo* mojom_db_transaction,
               int to_version) override;

 private:
  void MigrateToV43(mojom::DBTransactionInfo* mojom_db_transaction);

  std::string BuildInsertSql(mojom::DBActionInfo* mojom_db_action,
                             const CreativeAdList& creative_ads) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CREATIVE_ADS_DATABASE_TABLE_H_
