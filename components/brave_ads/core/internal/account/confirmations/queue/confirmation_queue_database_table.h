/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DATABASE_TABLE_H_

#include <string>

#include "base/check_op.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

namespace brave_ads::database::table {

using GetConfirmationQueueCallback = base::OnceCallback<void(
    bool success,
    const ConfirmationQueueItemList& confirmation_queue_items)>;

using GetConfirmationQueueForCreativeInstanceIdCallback =
    base::OnceCallback<void(
        bool success,
        const std::string& creative_instance_id,
        const ConfirmationQueueItemList& confirmation_queue_items)>;

class ConfirmationQueue final : public TableInterface {
 public:
  ConfirmationQueue();

  void Save(const ConfirmationQueueItemList& confirmation_queue_items,
            ResultCallback callback) const;

  void DeleteAll(ResultCallback callback) const;
  void Delete(const std::string& transaction_id, ResultCallback callback) const;

  void Retry(const std::string& transaction_id, ResultCallback callback) const;

  void GetAll(GetConfirmationQueueCallback callback) const;
  void GetNext(GetConfirmationQueueCallback callback) const;

  void SetBatchSize(const int batch_size) {
    CHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Create(mojom::DBTransactionInfo* mojom_db_transaction) override;
  void Migrate(mojom::DBTransactionInfo* mojom_db_transaction,
               int to_version) override;

 private:
  void Insert(mojom::DBTransactionInfo* mojom_db_transaction,
              const ConfirmationQueueItemList& confirmation_queue_items) const;

  std::string BuildInsertSql(
      mojom::DBActionInfo* mojom_db_action,
      const ConfirmationQueueItemList& confirmation_queue_items) const;

  int batch_size_;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DATABASE_TABLE_H_
