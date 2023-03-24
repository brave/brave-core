/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PROCESSED_PUBLISHER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PROCESSED_PUBLISHER_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::core {
namespace database {

class DatabaseProcessedPublisher : public DatabaseTable {
 public:
  explicit DatabaseProcessedPublisher(LedgerImpl* ledger);
  ~DatabaseProcessedPublisher() override;

  void InsertOrUpdateList(const std::vector<std::string>& list,
                          LegacyResultCallback callback);

  void WasProcessed(const std::string& publisher_key,
                    LegacyResultCallback callback);

 private:
  void OnWasProcessed(mojom::DBCommandResponsePtr response,
                      LegacyResultCallback callback);
};

}  // namespace database
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PROCESSED_PUBLISHER_H_
