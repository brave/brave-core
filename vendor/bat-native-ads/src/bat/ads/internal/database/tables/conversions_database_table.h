/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSIONS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSIONS_DATABASE_TABLE_H_

#include <functional>
#include <string>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/conversions/conversion_info.h"
#include "bat/ads/internal/database/database_table.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

using GetConversionsCallback =
    std::function<void(const bool, const ConversionList&)>;

namespace database {
namespace table {

class Conversions : public Table {
 public:
  Conversions();

  ~Conversions() override;

  void Save(const ConversionList& conversions, ResultCallback callback);

  void GetAll(GetConversionsCallback callback);

  void PurgeExpired(ResultCallback callback);

  std::string get_table_name() const override;

  void Migrate(mojom::DBTransaction* transaction,
               const int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransaction* transaction,
                      const ConversionList& conversion);

  int BindParameters(mojom::DBCommand* command,
                     const ConversionList& conversion);

  std::string BuildInsertOrUpdateQuery(mojom::DBCommand* command,
                                       const ConversionList& conversions);

  void OnGetConversions(mojom::DBCommandResponsePtr response,
                        GetConversionsCallback callback);

  ConversionInfo GetConversionFromRecord(mojom::DBRecord* record) const;

  void CreateTableV1(mojom::DBTransaction* transaction);
  void CreateIndexV1(mojom::DBTransaction* transaction);
  void MigrateToV1(mojom::DBTransaction* transaction);

  void MigrateToV10(mojom::DBTransaction* transaction);
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSIONS_DATABASE_TABLE_H_
