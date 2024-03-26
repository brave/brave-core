/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SKU_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SKU_TRANSACTION_H_

#include <string>

#include "base/types/expected.h"
#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

enum class GetSKUTransactionError { kDatabaseError, kTransactionNotFound };

using GetSKUTransactionCallback = base::OnceCallback<void(
    base::expected<mojom::SKUTransactionPtr, GetSKUTransactionError>)>;

class DatabaseSKUTransaction : public DatabaseTable {
 public:
  explicit DatabaseSKUTransaction(RewardsEngine& engine);
  ~DatabaseSKUTransaction() override;

  void InsertOrUpdate(mojom::SKUTransactionPtr info, ResultCallback callback);

  void SaveExternalTransaction(const std::string& transaction_id,
                               const std::string& external_transaction_id,
                               ResultCallback callback);

  void GetRecordByOrderId(const std::string& order_id,
                          GetSKUTransactionCallback callback);

 private:
  void OnGetRecord(GetSKUTransactionCallback callback,
                   mojom::DBCommandResponsePtr response);
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SKU_TRANSACTION_H_
