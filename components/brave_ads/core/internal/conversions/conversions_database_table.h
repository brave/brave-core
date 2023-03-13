/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "brave/components/brave_ads/core/ads_client_callback.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"

namespace brave_ads::database::table {

using GetConversionsCallback =
    base::OnceCallback<void(const bool, const ConversionList&)>;

class Conversions final : public TableInterface {
 public:
  void Save(const ConversionList& conversions, ResultCallback callback);

  void GetAll(GetConversionsCallback callback) const;

  void PurgeExpired(ResultCallback callback) const;

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const ConversionList& conversion);

  std::string BuildInsertOrUpdateQuery(mojom::DBCommandInfo* command,
                                       const ConversionList& conversions) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSIONS_DATABASE_TABLE_H_
