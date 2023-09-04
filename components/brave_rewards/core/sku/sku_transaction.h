/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_TRANSACTION_H_

#include <map>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/types/expected.h"
#include "brave/components/brave_rewards/core/database/database_external_transactions.h"
#include "brave/components/brave_rewards/core/database/database_sku_transaction.h"
#include "brave/components/brave_rewards/core/endpoint/payment/payment_server.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace sku {

class SKUTransaction {
 public:
  explicit SKUTransaction(RewardsEngineImpl& engine);
  ~SKUTransaction();

  void Run(mojom::SKUOrderPtr order,
           const std::string& destination,
           const std::string& wallet_type,
           LegacyResultCallback callback);

  void SendExternalTransaction(mojom::Result result,
                               const mojom::SKUTransaction& transaction,
                               LegacyResultCallback callback);

 private:
  using MaybeCreateTransactionCallback =
      std::function<void(mojom::Result, const mojom::SKUTransaction&)>;

  void MaybeCreateTransaction(mojom::SKUOrderPtr order,
                              const std::string& wallet_type,
                              MaybeCreateTransactionCallback callback);

  void OnGetSKUTransactionByOrderId(
      MaybeCreateTransactionCallback callback,
      const std::string& order_id,
      const std::string& wallet_type,
      double total_amount,
      base::expected<mojom::SKUTransactionPtr, database::GetSKUTransactionError>
          result);

  void OnTransactionSaved(mojom::Result result,
                          const mojom::SKUTransaction& transaction,
                          const std::string& destination,
                          const std::string& wallet_type,
                          const std::string& contribution_id,
                          LegacyResultCallback callback);

  void OnTransfer(mojom::Result result,
                  const mojom::SKUTransaction& transaction,
                  const std::string& contribution_id,
                  const std::string& destination,
                  LegacyResultCallback callback);

  void OnGetExternalTransaction(
      LegacyResultCallback,
      mojom::SKUTransaction&&,
      base::expected<mojom::ExternalTransactionPtr,
                     database::GetExternalTransactionError>);

  void OnSaveSKUExternalTransaction(mojom::Result result,
                                    const mojom::SKUTransaction& transaction,
                                    LegacyResultCallback callback);

  void OnSendExternalTransaction(mojom::Result result,
                                 LegacyResultCallback callback);

  const raw_ref<RewardsEngineImpl> engine_;
  endpoint::PaymentServer payment_server_;
};

}  // namespace sku
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_TRANSACTION_H_
