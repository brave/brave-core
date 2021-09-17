/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSION_QUEUE_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSION_QUEUE_DATABASE_TABLE_H_

#include <string>

#include "base/check_op.h"
#include "bat/ads/ads_client_aliases.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info_aliases.h"
#include "bat/ads/internal/database/database_table.h"
#include "bat/ads/internal/database/tables/conversion_queue_database_table_aliases.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct ConversionQueueItemInfo;

namespace database {
namespace table {

class ConversionQueue final : public Table {
 public:
  ConversionQueue();
  ~ConversionQueue() override;

  void Save(const ConversionQueueItemList& conversion_queue_items,
            ResultCallback callback);

  void Delete(const ConversionQueueItemInfo& conversion_queue_item,
              ResultCallback callback);

  void GetAll(GetConversionQueueCallback callback);

  void GetForCreativeInstanceId(
      const std::string& creative_instance_id,
      GetConversionQueueForCreativeInstanceIdCallback callback);

  void set_batch_size(const int batch_size) {
    DCHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransaction* transaction,
               const int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransaction* transaction,
                      const ConversionQueueItemList& conversion_queue_items);

  int BindParameters(mojom::DBCommand* command,
                     const ConversionQueueItemList& conversion_queue_items);

  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommand* command,
      const ConversionQueueItemList& conversion_queue_items);

  void OnGetAll(mojom::DBCommandResponsePtr response,
                GetConversionQueueCallback callback);

  void OnGetForCreativeInstanceId(
      mojom::DBCommandResponsePtr response,
      const std::string& creative_instance_id,
      GetConversionQueueForCreativeInstanceIdCallback callback);

  ConversionQueueItemInfo GetFromRecord(mojom::DBRecord* record) const;

  void CreateTableV10(mojom::DBTransaction* transaction);
  void MigrateToV10(mojom::DBTransaction* transaction);

  void MigrateToV11(mojom::DBTransaction* transaction);

  int batch_size_;
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSION_QUEUE_DATABASE_TABLE_H_
