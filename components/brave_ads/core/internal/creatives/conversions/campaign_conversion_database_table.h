/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_DATABASE_TABLE_H_

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads::database::table {

using GetCampaignConversionsCallback = base::OnceCallback<
    void(bool success, const CampaignConversionList& campaign_conversions)>;

class CampaignConversions final : public TableInterface {
 public:
  void Save(const CampaignConversionList& campaign_conversions,
            ResultCallback callback);

  void GetUnexpired(GetCampaignConversionsCallback callback) const;
  void GetActive(GetCampaignConversionsCallback callback) const;

  void PurgeExpired(ResultCallback callback) const;

  // TableInterface:
  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CAMPAIGN_CONVERSION_DATABASE_TABLE_H_
