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
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "sql/statement.h"

namespace sql {
class Database;
}

namespace brave_wallet {

template <size_t const T>
base::expected<std::optional<std::array<uint8_t, T>>, std::string>
ReadSizedBlob(sql::Statement& statement, size_t position) {
  auto columns = statement.ColumnCount();
  CHECK(columns >= 0);
  if (position >= static_cast<size_t>(columns)) {
    return base::unexpected("Position mismatch");
  }

  if (statement.GetColumnType(position) == sql::ColumnType::kNull) {
    return std::nullopt;
  }

  if (statement.GetColumnType(position) != sql::ColumnType::kBlob) {
    return base::unexpected("Type mismatch");
  }

  auto blob = statement.ColumnBlob(position);
  if (blob.size() != T) {
    return base::unexpected("Size mismatch");
  }

  std::array<uint8_t, T> to;
  base::ranges::copy_n(blob.begin(), to.size(), to.begin());
  return to;
}

// Implements SQLite database to store found incoming notes,
// nullifiers, wallet zcash accounts and commitment trees.
class ZCashOrchardStorage : public base::RefCounted<ZCashOrchardStorage> {
 public:
  using WithCheckpointsCallback =
      base::RepeatingCallback<void(uint32_t, OrchardCheckpoint checkpoint)>;

  struct AccountMeta {
    AccountMeta();
    ~AccountMeta();
    AccountMeta(const AccountMeta&);
    AccountMeta& operator=(const AccountMeta&);
    uint32_t account_birthday = 0;
    std::optional<uint32_t> latest_scanned_block_id;
    std::optional<std::string> latest_scanned_block_hash;
  };

  enum class ErrorCode {
    kDbInitError,
    kAccountNotFound,
    kFailedToExecuteStatement,
    kFailedToCommitTransaction,
    kInternalError,
    kNoCheckpoints,
    kConsistencyError
  };

  struct Error {
    ErrorCode error_code;
    std::string message;
  };

  explicit ZCashOrchardStorage(base::FilePath path_to_database);

  base::expected<AccountMeta, Error> RegisterAccount(
      mojom::AccountIdPtr account_id,
      uint32_t account_birthday_block);
  base::expected<AccountMeta, Error> GetAccountMeta(
      mojom::AccountIdPtr account_id);
  base::expected<bool, Error> ResetAccountSyncState(
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
  base::expected<std::vector<OrchardNoteSpend>, Error> GetNullifiers(
      mojom::AccountIdPtr account_id);
  // Updates database with discovered spendable notes and nullifiers
  // Also updates account info with latest scanned block info
  std::optional<Error> UpdateNotes(
      mojom::AccountIdPtr account_id,
      const std::vector<OrchardNote>& notes_to_add,
      const std::vector<OrchardNoteSpend>& found_nullifiers,
      const uint32_t latest_scanned_block,
      const std::string& latest_scanned_block_hash);
  void ResetDatabase();

  // Shard tree
  base::expected<std::optional<OrchardCap>, Error> GetCap(
      mojom::AccountIdPtr account_id);
  base::expected<bool, Error> PutCap(mojom::AccountIdPtr account_id,
                                     OrchardCap cap);

  base::expected<bool, Error> TruncateShards(mojom::AccountIdPtr account_id,
                                             uint32_t shard_index);
  base::expected<std::optional<uint32_t>, Error> GetLatestShardIndex(
      mojom::AccountIdPtr account_id);
  base::expected<bool, Error> PutShard(mojom::AccountIdPtr account_id,
                                       OrchardShard shard);
  base::expected<std::optional<OrchardShard>, Error> GetShard(
      mojom::AccountIdPtr account_id,
      OrchardShardAddress address);
  base::expected<std::optional<OrchardShard>, Error> LastShard(
      mojom::AccountIdPtr account_id,
      uint8_t shard_height);

  base::expected<size_t, Error> CheckpointCount(mojom::AccountIdPtr account_id);
  base::expected<std::optional<uint32_t>, Error> MinCheckpointId(
      mojom::AccountIdPtr account_id);
  base::expected<std::optional<uint32_t>, Error> MaxCheckpointId(
      mojom::AccountIdPtr account_id);
  base::expected<std::optional<uint32_t>, Error> GetCheckpointAtDepth(
      mojom::AccountIdPtr account_id,
      uint32_t depth);
  base::expected<std::optional<uint32_t>, Error> GetMaxCheckpointedHeight(
      mojom::AccountIdPtr account_id,
      uint32_t chain_tip_height,
      uint32_t min_confirmations);
  base::expected<bool, Error> RemoveCheckpoint(mojom::AccountIdPtr account_id,
                                               uint32_t checkpoint_id);
  base::expected<bool, Error> TruncateCheckpoints(
      mojom::AccountIdPtr account_id,
      uint32_t checkpoint_id);
  base::expected<bool, Error> AddCheckpoint(mojom::AccountIdPtr account_id,
                                            uint32_t checkpoint_id,
                                            OrchardCheckpoint checkpoint);
  base::expected<std::vector<OrchardCheckpointBundle>, Error> GetCheckpoints(
      mojom::AccountIdPtr account_id,
      size_t limit);
  base::expected<std::optional<OrchardCheckpointBundle>, Error> GetCheckpoint(
      mojom::AccountIdPtr account_id,
      uint32_t checkpoint_id);
  base::expected<std::optional<std::vector<uint32_t>>, Error> GetMarksRemoved(
      mojom::AccountIdPtr account_id,
      uint32_t checkpoint_id);

  base::expected<bool, Error> UpdateSubtreeRoots(
      mojom::AccountIdPtr account_id,
      uint32_t start_index,
      std::vector<zcash::mojom::SubtreeRootPtr> roots);
  base::expected<std::vector<OrchardShardAddress>, Error> GetShardRoots(
      mojom::AccountIdPtr account_id,
      uint8_t shard_level);

 private:
  friend class base::RefCounted<ZCashOrchardStorage>;

  ~ZCashOrchardStorage();

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
