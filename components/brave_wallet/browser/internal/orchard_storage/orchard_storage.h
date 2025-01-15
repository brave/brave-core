/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_STORAGE_ORCHARD_STORAGE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_STORAGE_ORCHARD_STORAGE_H_

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_shard_tree_types.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "sql/database.h"
#include "sql/statement.h"
#include "sql/transaction.h"

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
    return base::ok(std::nullopt);
  }

  if (statement.GetColumnType(position) != sql::ColumnType::kBlob) {
    return base::unexpected("Type mismatch");
  }

  auto blob = statement.ColumnBlob(position);
  if (blob.size() != T) {
    return base::unexpected("Size mismatch");
  }

  std::array<uint8_t, T> to;
  base::span(to).copy_from(blob);
  return base::ok(to);
}

// Implements SQLite database to store found incoming notes,
// nullifiers, wallet zcash accounts and commitment trees.
class OrchardStorage {
 public:
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
    kFailedToExecuteStatement,
    kFailedToCreateTransaction,
    kFailedToCommitTransaction,
    kInternalError,
    kConsistencyError
  };

  enum class Result { kSuccess, kNone };

  struct Error {
    ErrorCode error_code;
    std::string message;
  };

  class TransactionScope {
   public:
    explicit TransactionScope(sql::Database& database);
    ~TransactionScope();
    TransactionScope(const TransactionScope&) = delete;
    TransactionScope& operator=(const TransactionScope&) = delete;
    TransactionScope(TransactionScope&&);
    TransactionScope& operator=(TransactionScope&&);

    base::expected<Result, Error> Commit();
    base::expected<void, Error> Begin(
        base::PassKey<class OrchardStorage> pass_key);

   private:
    raw_ref<sql::Database> database_ GUARDED_BY_CONTEXT(sequence_checker_);
    // Transaction doesn't have move constructor so wrap it with unique_tr
    std::unique_ptr<sql::Transaction> transaction_;
    SEQUENCE_CHECKER(sequence_checker_);
  };

  explicit OrchardStorage(const base::FilePath& path_to_database);
  ~OrchardStorage();

  base::expected<Result, Error> RegisterAccount(
      const mojom::AccountIdPtr& account_id,
      uint32_t account_birthday_block);
  base::expected<std::optional<AccountMeta>, Error> GetAccountMeta(
      const mojom::AccountIdPtr& account_id);
  base::expected<Result, Error> ResetAccountSyncState(
      const mojom::AccountIdPtr& account_id);

  // Removes database records which are under effect of chain reorg
  // Removes spendable notes and nullifiers with block_height > reorg_block
  // Updates account's last scanned block to chain reorg block
  base::expected<Result, Error> HandleChainReorg(
      const mojom::AccountIdPtr& account_id,
      uint32_t reorg_block_id,
      const std::string& reorg_block_hash);
  // Calculates a list of discovered spendable notes that don't have nullifiers
  // in the blockchain
  base::expected<std::vector<OrchardNote>, OrchardStorage::Error>
  GetSpendableNotes(const mojom::AccountIdPtr& account_id);
  // Returns a list of discovered nullifiers
  base::expected<std::vector<OrchardNoteSpend>, Error> GetNullifiers(
      const mojom::AccountIdPtr& account_id);
  // Updates database with discovered spendable notes and nullifiers
  // Also updates account info with latest scanned block info
  base::expected<Result, Error> UpdateNotes(
      const mojom::AccountIdPtr& account_id,
      base::span<const OrchardNote> notes_to_add,
      base::span<const OrchardNoteSpend> found_nullifiers,
      const uint32_t latest_scanned_block,
      const std::string& latest_scanned_block_hash);
  void ResetDatabase();

  // Shard tree
  base::expected<std::optional<OrchardShardTreeCap>, Error> GetCap(
      const mojom::AccountIdPtr& account_id);
  base::expected<Result, Error> PutCap(const mojom::AccountIdPtr& account_id,
                                       const OrchardShardTreeCap& cap);

  base::expected<Result, Error> TruncateShards(
      const mojom::AccountIdPtr& account_id,
      uint32_t shard_index);
  base::expected<std::optional<uint32_t>, Error> GetLatestShardIndex(
      const mojom::AccountIdPtr& account_id);
  base::expected<Result, Error> PutShard(const mojom::AccountIdPtr& account_id,
                                         const OrchardShard& shard);
  base::expected<std::optional<OrchardShard>, Error> GetShard(
      const mojom::AccountIdPtr& account_id,
      const OrchardShardAddress& address);
  base::expected<std::optional<OrchardShard>, Error> LastShard(
      const mojom::AccountIdPtr& account_id,
      uint8_t shard_height);

  base::expected<size_t, Error> CheckpointCount(
      const mojom::AccountIdPtr& account_id);
  base::expected<std::optional<uint32_t>, Error> MinCheckpointId(
      const mojom::AccountIdPtr& account_id);
  base::expected<std::optional<uint32_t>, Error> MaxCheckpointId(
      const mojom::AccountIdPtr& account_id);
  base::expected<std::optional<uint32_t>, Error> GetCheckpointAtDepth(
      const mojom::AccountIdPtr& account_id,
      uint32_t depth);
  base::expected<std::optional<uint32_t>, Error> GetMaxCheckpointedHeight(
      const mojom::AccountIdPtr& account_id,
      uint32_t chain_tip_height,
      uint32_t min_confirmations);
  base::expected<Result, Error> RemoveCheckpoint(
      const mojom::AccountIdPtr& account_id,
      uint32_t checkpoint_id);
  base::expected<Result, Error> TruncateCheckpoints(
      const mojom::AccountIdPtr& account_id,
      uint32_t checkpoint_id);
  base::expected<Result, Error> AddCheckpoint(
      const mojom::AccountIdPtr& account_id,
      uint32_t checkpoint_id,
      const OrchardCheckpoint& checkpoint);
  base::expected<Result, Error> UpdateCheckpoint(
      const mojom::AccountIdPtr& account_id,
      uint32_t checkpoint_id,
      const OrchardCheckpoint& checkpoint);
  base::expected<std::vector<OrchardCheckpointBundle>, Error> GetCheckpoints(
      const mojom::AccountIdPtr& account_id,
      size_t limit);
  base::expected<std::optional<OrchardCheckpointBundle>, Error> GetCheckpoint(
      const mojom::AccountIdPtr& account_id,
      uint32_t checkpoint_id);
  base::expected<std::vector<uint32_t>, Error> GetMarksRemoved(
      const mojom::AccountIdPtr& account_id,
      uint32_t checkpoint_id);

  base::expected<Result, Error> UpdateSubtreeRoots(
      const mojom::AccountIdPtr& account_id,
      uint32_t start_index,
      const std::vector<zcash::mojom::SubtreeRootPtr>& roots);
  base::expected<std::vector<OrchardShardAddress>, Error> GetShardRoots(
      const mojom::AccountIdPtr& account_id,
      uint8_t shard_level);

  base::expected<TransactionScope, Error> Transactionally();
  bool EnsureDbInit();

 private:
  bool CreateOrUpdateDatabase();
  bool CreateSchema();
  bool UpdateSchema();

  base::FilePath db_file_path_;
  sql::Database database_ GUARDED_BY_CONTEXT(sequence_checker_);

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_INTERNAL_ORCHARD_STORAGE_ORCHARD_STORAGE_H_
