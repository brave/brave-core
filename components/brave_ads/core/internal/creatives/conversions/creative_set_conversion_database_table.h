/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

namespace brave_ads::database::table {

using GetCreativeSetConversionsCallback = base::OnceCallback<void(
    bool success,
    const CreativeSetConversionList& creative_set_conversions)>;

class CreativeSetConversions final : public TableInterface {
 public:
  void Save(const CreativeSetConversionList& creative_set_conversions,
            ResultCallback callback);

  void GetUnexpired(GetCreativeSetConversionsCallback callback) const;

  void PurgeExpired(ResultCallback callback) const;

  std::string GetTableName() const override;

  void Create(mojom::DBTransactionInfo* mojom_transaction) override;
  void Migrate(mojom::DBTransactionInfo* mojom_transaction,
               int to_version) override;

 private:
  void Insert(mojom::DBTransactionInfo* mojom_transaction,
              const CreativeSetConversionList& creative_set_conversions);

  std::string BuildInsertSql(
      mojom::DBStatementInfo* mojom_statement,
      const CreativeSetConversionList& creative_set_conversions) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_DATABASE_TABLE_H_
