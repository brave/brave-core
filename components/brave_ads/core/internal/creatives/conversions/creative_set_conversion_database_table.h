/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/ads_client_callback.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"

namespace brave_ads::database::table {

using GetConversionsCallback = base::OnceCallback<void(
    bool success,
    const CreativeSetConversionList& creative_set_conversions)>;

class CreativeSetConversions final : public TableInterface {
 public:
  void Save(const CreativeSetConversionList& creative_set_conversions,
            ResultCallback callback);

  void GetAll(GetConversionsCallback callback) const;

  void PurgeExpired(ResultCallback callback) const;

  std::string GetTableName() const override;

  void Create(mojom::DBTransactionInfo* transaction) override;
  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  void InsertOrUpdate(
      mojom::DBTransactionInfo* transaction,
      const CreativeSetConversionList& creative_set_conversions);

  std::string BuildInsertOrUpdateSql(
      mojom::DBCommandInfo* command,
      const CreativeSetConversionList& creative_set_conversions) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_DATABASE_TABLE_H_
