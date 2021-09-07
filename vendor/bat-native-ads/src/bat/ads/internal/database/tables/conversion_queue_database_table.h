/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSION_QUEUE_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSION_QUEUE_DATABASE_TABLE_H_

#include <functional>
#include <string>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/database/database_table.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

using GetConversionQueueCallback =
    std::function<void(const bool, const ConversionQueueItemList&)>;

using GetConversionQueueForCreativeInstanceIdCallback =
    std::function<void(const bool,
                       const std::string& creative_instance_id,
                       const ConversionQueueItemList&)>;

namespace database {
namespace table {

class ConversionQueue : public Table {
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

  void set_batch_size(const int batch_size);

  std::string get_table_name() const override;

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
