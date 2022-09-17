/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_H_

#include <string>

#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/account/transactions/transaction_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace base {
class Time;
}  // namespace base

namespace ads::database::table {

using GetTransactionsCallback =
    std::function<void(const bool, const TransactionList&)>;

class Transactions final : public TableInterface {
 public:
  Transactions();

  Transactions(const Transactions& other) = delete;
  Transactions& operator=(const Transactions& other) = delete;

  Transactions(Transactions&& other) noexcept = delete;
  Transactions& operator=(Transactions&& other) noexcept = delete;

  ~Transactions() override;

  void Save(const TransactionList& transactions, ResultCallback callback);

  void GetAll(GetTransactionsCallback callback);
  void GetForDateRange(base::Time from_time,
                       base::Time to_time,
                       GetTransactionsCallback callback);

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

  void OnGetTransactions(const GetTransactionsCallback& callback,
                         mojom::DBCommandResponseInfoPtr response);

  void MigrateToV18(mojom::DBTransactionInfo* transaction);
};

}  // namespace ads::database::table

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_H_
