/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_DATABASE_TABLE_H_

#include <functional>
#include <string>

#include "base/check_op.h"
#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace database {
namespace table {

using GetConversionQueueCallback =
    std::function<void(const bool, const ConversionQueueItemList&)>;

using GetConversionQueueForCreativeInstanceIdCallback =
    std::function<void(const bool,
                       const std::string& creative_instance_id,
                       const ConversionQueueItemList&)>;

class ConversionQueue final : public TableInterface {
 public:
  ConversionQueue();
  ~ConversionQueue() override;
  ConversionQueue(const ConversionQueue&) = delete;
  ConversionQueue& operator=(const ConversionQueue&) = delete;

  void Save(const ConversionQueueItemList& conversion_queue_items,
            ResultCallback callback);

  void Delete(const ConversionQueueItemInfo& conversion_queue_item,
              ResultCallback callback);

  void Update(const ConversionQueueItemInfo& conversion_queue_item,
              ResultCallback callback);

  void GetAll(GetConversionQueueCallback callback);

  void GetUnprocessed(GetConversionQueueCallback callback);

  void GetForCreativeInstanceId(
      const std::string& creative_instance_id,
      GetConversionQueueForCreativeInstanceIdCallback callback);

  void SetBatchSize(const int batch_size) {
    DCHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction,
               const int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const ConversionQueueItemList& conversion_queue_items);

  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommandInfo* command,
      const ConversionQueueItemList& conversion_queue_items);

  void OnGetAll(mojom::DBCommandResponseInfoPtr response,
                GetConversionQueueCallback callback);

  void OnGetForCreativeInstanceId(
      mojom::DBCommandResponseInfoPtr response,
      const std::string& creative_instance_id,
      GetConversionQueueForCreativeInstanceIdCallback callback);

  void MigrateToV10(mojom::DBTransactionInfo* transaction);
  void MigrateToV11(mojom::DBTransactionInfo* transaction);
  void MigrateToV17(mojom::DBTransactionInfo* transaction);
  void MigrateToV21(mojom::DBTransactionInfo* transaction);

  int batch_size_;
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_DATABASE_TABLE_H_
