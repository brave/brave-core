/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads::database::table {

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

}  // namespace ads::database::table

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSIONS_DATABASE_TABLE_H_
