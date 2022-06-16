/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_H_

#include <string>

#include "bat/ads/ads_client_aliases.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info_aliases.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "bat/ads/transaction_info_aliases.h"

namespace base {
class Time;
}  // namespace base

namespace ads {
namespace database {
namespace table {

using GetTransactionsCallback =
    std::function<void(const bool, const TransactionList&)>;

class Transactions final : public TableInterface {
 public:
  Transactions();
  ~Transactions() override;
  Transactions(const Transactions&) = delete;
  Transactions& operator=(const Transactions&) = delete;

  void Save(const TransactionList& transactions, ResultCallback callback);

  void GetAll(GetTransactionsCallback callback);
  void GetForDateRange(const base::Time from_time,
                       const base::Time to_time,
                       GetTransactionsCallback callback);

  void Update(
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
      ResultCallback callback);

  void Delete(ResultCallback callback);

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransaction* transaction,
               const int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransaction* transaction,
                      const TransactionList& transactions);

  std::string BuildInsertOrUpdateQuery(mojom::DBCommand* command,
                                       const TransactionList& transactions);

  void OnGetTransactions(mojom::DBCommandResponsePtr response,
                         GetTransactionsCallback callback);

  void MigrateToV18(mojom::DBTransaction* transaction);
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_TRANSACTIONS_TRANSACTIONS_DATABASE_TABLE_H_
