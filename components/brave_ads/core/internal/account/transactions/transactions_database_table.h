/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_H_

#include <string>
#include <utility>

#include "base/functional/callback_forward.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"
#include "brave/components/brave_ads/core/ads_client_callback.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads::database::table {

using GetTransactionsCallback =
    base::OnceCallback<void(bool success, const TransactionList& transactions)>;

class Transactions final : public TableInterface {
 public:
  void Save(const TransactionList& transactions, ResultCallback callback);

  void GetAll(GetTransactionsCallback callback) const;
  void GetForDateRange(base::Time from_time,
                       base::Time to_time,
                       GetTransactionsCallback callback) const;

  void Update(
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
      ResultCallback callback) const;

  void Delete(ResultCallback callback) const;

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const TransactionList& transactions);

  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommandInfo* command,
      const TransactionList& transactions) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_H_
