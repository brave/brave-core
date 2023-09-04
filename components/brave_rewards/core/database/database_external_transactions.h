/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_EXTERNAL_TRANSACTIONS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_EXTERNAL_TRANSACTIONS_H_

#include <string>

#include "base/types/expected.h"
#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal::database {

enum class GetExternalTransactionError { kDatabaseError, kTransactionNotFound };

using GetExternalTransactionCallback =
    base::OnceCallback<void(base::expected<mojom::ExternalTransactionPtr,
                                           GetExternalTransactionError>)>;

class DatabaseExternalTransactions : public DatabaseTable {
 public:
  explicit DatabaseExternalTransactions(RewardsEngineImpl& engine);
  ~DatabaseExternalTransactions() override;

  void Insert(mojom::ExternalTransactionPtr, ResultCallback);
  void GetTransaction(const std::string& contribution_id,
                      const std::string& destination,
                      GetExternalTransactionCallback);

 private:
  void OnInsert(ResultCallback, mojom::DBCommandResponsePtr);
  void OnGetTransaction(GetExternalTransactionCallback,
                        mojom::DBCommandResponsePtr);
};

}  // namespace brave_rewards::internal::database

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_EXTERNAL_TRANSACTIONS_H_
