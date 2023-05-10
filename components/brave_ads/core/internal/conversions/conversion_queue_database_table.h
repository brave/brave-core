/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_DATABASE_TABLE_H_

#include <string>

#include "base/check_op.h"
#include "base/functional/callback_forward.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/ads_client_callback.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"

namespace brave_ads::database::table {

using GetConversionQueueCallback = base::OnceCallback<
    void(bool success, const ConversionQueueItemList& conversion_queue_items)>;

using GetConversionQueueForCreativeInstanceIdCallback = base::OnceCallback<void(
    bool success,
    const std::string& creative_instance_id,
    const ConversionQueueItemList& conversion_queue_items)>;

class ConversionQueue final : public TableInterface {
 public:
  ConversionQueue();

  void Save(const ConversionQueueItemList& conversion_queue_items,
            ResultCallback callback);

  void Delete(const ConversionQueueItemInfo& conversion_queue_item,
              ResultCallback callback) const;

  void Update(const ConversionQueueItemInfo& conversion_queue_item,
              ResultCallback callback) const;

  void GetAll(GetConversionQueueCallback callback) const;

  void GetUnprocessed(GetConversionQueueCallback callback) const;

  void GetForCreativeInstanceId(
      const std::string& creative_instance_id,
      GetConversionQueueForCreativeInstanceIdCallback callback) const;

  void SetBatchSize(const int batch_size) {
    CHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Create(mojom::DBTransactionInfo* transaction) override;
  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const ConversionQueueItemList& conversion_queue_items);

  std::string BuildInsertOrUpdateSql(
      mojom::DBCommandInfo* command,
      const ConversionQueueItemList& conversion_queue_items) const;

  int batch_size_;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_DATABASE_TABLE_H_
