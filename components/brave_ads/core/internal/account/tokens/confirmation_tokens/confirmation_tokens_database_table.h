/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_DATABASE_TABLE_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads::database::table {

using GetConfirmationTokensCallback =
    base::OnceCallback<void(bool success,
                            const ConfirmationTokenList& confirmation_tokens)>;

// Persists unspent confirmation tokens that are pending assignment to an ad
// confirmation. Tokens are loaded from this table on startup to populate the
// in-memory cache, and written here on every add or removal.
class ConfirmationTokens final : public TableInterface {
 public:
  void Save(const ConfirmationTokenList& confirmation_tokens,
            ResultCallback callback);

  void Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
              const ConfirmationTokenList& confirmation_tokens);

  void Delete(const ConfirmationTokenInfo& confirmation_token,
              ResultCallback callback);
  void DeleteAll(ResultCallback callback);

  void GetAll(GetConfirmationTokensCallback callback) const;

  // TableInterface:
  void Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) override;
  void Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               int to_version) override;

 private:
  std::string BuildInsertSql(
      const mojom::DBActionInfoPtr& mojom_db_action,
      const ConfirmationTokenList& confirmation_tokens) const;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_DATABASE_TABLE_H_
