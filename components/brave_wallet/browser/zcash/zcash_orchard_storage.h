/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_ORCHARD_STORAGE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_ORCHARD_STORAGE_H_

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace sql {
class Database;
}

namespace brave_wallet {

// Implements SQLite database to store found incoming notes,
// nullifiers, wallet zcash accounts and commitment trees.
class ZCashOrchardStorage {
 public:
  struct AccountMeta {
    uint32_t account_birthday = 0;
    uint32_t latest_scanned_block_id = 0;
    std::string latest_scanned_block_hash;
  };

  enum class ErrorCode {
    kDbInitError,
    kAccountNotFound,
    kFailedToExecuteStatement,
    kInternalError
  };

  struct Error {
    ErrorCode error_code;
    std::string message;
  };

  explicit ZCashOrchardStorage(base::FilePath path_to_database);
  ~ZCashOrchardStorage();

  base::expected<AccountMeta, Error> RegisterAccount(
      mojom::AccountIdPtr account_id,
      uint32_t account_birthday_block,
      const std::string& account_bithday_block_hash);
  base::expected<AccountMeta, Error> GetAccountMeta(
      mojom::AccountIdPtr account_id);

  // Removes database records which are under effect of chain reorg
  // Removes spendable notes and nullifiers with block_height > reorg_block
  // Updates account's last scanned block to chain reorg block
  std::optional<Error> HandleChainReorg(mojom::AccountIdPtr account_id,
                                        uint32_t reorg_block_id,
                                        const std::string& reorg_block_hash);
  // Calculates a list of discovered spendable notes that don't have nullifiers
  // in the blockchain
  base::expected<std::vector<OrchardNote>, ZCashOrchardStorage::Error>
  GetSpendableNotes(mojom::AccountIdPtr account_id);
  // Returns a list of discovered nullifiers
  base::expected<std::vector<OrchardNullifier>, Error> GetNullifiers(
      mojom::AccountIdPtr account_id);
  // Updates database with discovered spendable notes and nullifiers
  // Also updates account info with latest scanned block info
  std::optional<Error> UpdateNotes(
      mojom::AccountIdPtr account_id,
      const std::vector<OrchardNote>& notes_to_add,
      const std::vector<OrchardNullifier>& notes_to_delete,
      const uint32_t latest_scanned_block,
      const std::string& latest_scanned_block_hash);
  void ResetDatabase();

 private:
  bool EnsureDbInit();
  bool CreateOrUpdateDatabase();
  bool CreateSchema();
  bool UpdateSchema();

  base::FilePath db_file_path_;
  std::unique_ptr<sql::Database> database_
      GUARDED_BY_CONTEXT(sequence_checker_);

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_ORCHARD_STORAGE_H_
