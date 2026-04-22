/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads::database::table {

using GetPaymentTokensCallback =
    base::OnceCallback<void(bool success,
                            const PaymentTokenList& payment_tokens)>;

// Persists unspent payment tokens that have been earned via ad confirmations
// and are awaiting redemption. Tokens are loaded from this table on startup to
// populate the in-memory cache, and written here on every add or removal.
class PaymentTokens final : public TableInterface {
 public:
  void Save(const PaymentTokenList& payment_tokens, ResultCallback callback);

  void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
              const PaymentTokenList& payment_tokens);

  void Delete(const PaymentTokenInfo& payment_token, ResultCallback callback);
  void Delete(const PaymentTokenList& payment_tokens, ResultCallback callback);
  void DeleteAll(ResultCallback callback);

  void GetAll(GetPaymentTokensCallback callback) const;

  // TableInterface:
  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  std::string BuildInsertSql(const mojom::DBActionInfoPtr& mojom_db_action,
                             const PaymentTokenList& payment_tokens) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_PAYMENT_TOKENS_PAYMENT_TOKENS_DATABASE_TABLE_H_
