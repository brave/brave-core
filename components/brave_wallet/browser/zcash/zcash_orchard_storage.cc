/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_orchard_storage.h"

#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_wallet {

namespace {
#define kNotesTable "notes"
#define kSpentNotesTable "spent_notes"
#define kAccountMeta "account_meta"
#define kShardTree "shard_tree"
#define kShardTreeCheckpoints "checkpoints"
#define kCheckpointsMarksRemoved "checkpoints_mark_removed"

const int kEmptyDbVersionNumber = 1;
const int kCurrentVersionNumber = 2;

std::optional<uint32_t> ReadUint32(sql::Statement& statement, size_t index) {
  auto v = statement.ColumnInt64(index);
  if (v < 0 || v > std::numeric_limits<uint32_t>::max()) {
    return std::nullopt;
  }
  return static_cast<uint32_t>(v);
}

}  // namespace

ZCashOrchardStorage::ZCashOrchardStorage(base::FilePath path_to_database)
    : db_file_path_(path_to_database), database_() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

ZCashOrchardStorage::~ZCashOrchardStorage() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool ZCashOrchardStorage::EnsureDbInit() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (database_.is_open()) {
    return true;
  }
  return CreateOrUpdateDatabase();
}

void ZCashOrchardStorage::ResetDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  database_.Close();
  sql::Database::Delete(db_file_path_);
}

bool ZCashOrchardStorage::CreateOrUpdateDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const base::FilePath dir = db_file_path_.DirName();
  if (!base::DirectoryExists(dir) && !base::CreateDirectory(dir)) {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  if (!database_.Open(db_file_path_)) {
    NOTREACHED_IN_MIGRATION();
    return false;
  }

  sql::MetaTable meta_table;
  if (!meta_table.Init(&database_, kEmptyDbVersionNumber,
                       kEmptyDbVersionNumber)) {
    NOTREACHED_IN_MIGRATION();
    database_.Close();
    return false;
  }

  if (meta_table.GetVersionNumber() == kEmptyDbVersionNumber) {
    if (!CreateSchema() ||
        !meta_table.SetVersionNumber(kCurrentVersionNumber)) {
      NOTREACHED_IN_MIGRATION();
      database_.Close();
      return false;
    }
  } else if (meta_table.GetVersionNumber() < kCurrentVersionNumber) {
    if (!UpdateSchema() ||
        !meta_table.SetVersionNumber(kCurrentVersionNumber)) {
      NOTREACHED_IN_MIGRATION();
      database_.Close();
      return false;
    }
  }

  return true;
}

bool ZCashOrchardStorage::CreateSchema() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sql::Transaction transaction(&database_);
  return transaction.Begin() &&
         database_.Execute("CREATE TABLE " kNotesTable
                           " "
                           "("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "account_id TEXT NOT NULL,"
                           "amount INTEGER NOT NULL,"
                           "block_id INTEGER NOT NULL,"
                           "nullifier BLOB NOT NULL UNIQUE);") &&
         database_.Execute("CREATE TABLE " kSpentNotesTable
                           " "
                           "("
                           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "account_id TEXT NOT NULL,"
                           "spent_block_id INTEGER NOT NULL,"
                           "nullifier BLOB NOT NULL UNIQUE);") &&
         database_.Execute("CREATE TABLE " kAccountMeta
                           " "
                           "("
                           "account_id TEXT NOT NULL PRIMARY KEY,"
                           "account_birthday INTEGER NOT NULL,"
                           "latest_scanned_block INTEGER NOT NULL,"
                           "latest_scanned_block_hash TEXT NOT NULL);") &&
         database_.Execute("CREATE TABLE " kShardTree
                           " "
                           "("
                           "shard_index INTEGER PRIMARY KEY,"
                           "subtree_end_height INTEGER,"
                           "root_hash BLOB,"
                           "shard_data BLOB,"
                           "contains_marked INTEGER,"
                           "CONSTRAINT root_unique UNIQUE (root_hash));") &&
         database_.Execute("CREATE TABLE " kShardTreeCheckpoints
                           " ("
                           "checkpoint_id INTEGER PRIMARY KEY,"
                           "position INTEGER)") &&
         database_.Execute("CREATE TABLE " kCheckpointsMarksRemoved " ("
                           "checkpoint_id INTEGER NOT NULL,"
                           "mark_removed_position INTEGER NOT NULL,"
                           "FOREIGN KEY (checkpoint_id) REFERENCES orchard_tree_checkpoints(checkpoint_id)"
                           "ON DELETE CASCADE,"
                           "CONSTRAINT spend_position_unique UNIQUE (checkpoint_id, mark_removed_position)"
                           ")") &&
         transaction.Commit();
}

bool ZCashOrchardStorage::UpdateSchema() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return true;
}

std::optional<ZCashOrchardStorage::Error> ZCashOrchardStorage::RegisterAccount(
    mojom::AccountIdPtr account_id,
    uint64_t account_birthday_block,
    const std::string& account_birthday_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return Error{kDbInitError, "Failed to init database "};
  }

  sql::Transaction transaction(&database_);
  if (!transaction.Begin()) {
    NOTREACHED_IN_MIGRATION();
    return Error{kDbInitError, "Failed to init database "};
  }

  sql::Statement register_account_statement(database_.GetCachedStatement(
      SQL_FROM_HERE, "INSERT OR REPLACE INTO " kAccountMeta " "
                     "(account_id, account_birthday, latest_scanned_block, "
                     "latest_scanned_block_hash) "
                     "VALUES (?, ?, ?, ?)"));

  register_account_statement.BindString(0, account_id->unique_key);
  register_account_statement.BindInt64(1, account_birthday_block);
  register_account_statement.BindInt64(2, account_birthday_block);
  register_account_statement.BindString(3, account_birthday_block_hash);

  if (!register_account_statement.Run()) {
    transaction.Rollback();
    NOTREACHED_IN_MIGRATION();
    return Error{kFailedToExecuteStatement, database_.GetErrorMessage()};
  }

  if (!transaction.Commit()) {
    return Error{kFailedToExecuteStatement, database_.GetErrorMessage()};
  }

  return std::nullopt;
}

base::expected<ZCashOrchardStorage::AccountMeta, ZCashOrchardStorage::Error>
ZCashOrchardStorage::GetAccountMeta(mojom::AccountIdPtr account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  sql::Statement resolve_account_statement(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT account_birthday, latest_scanned_block, "
      "latest_scanned_block_hash FROM " kAccountMeta " WHERE account_id = ?;"));

  resolve_account_statement.BindString(0, account_id->unique_key);

  if (!resolve_account_statement.Step()) {
    return base::unexpected(Error{kAccountNotFound, "Account not found"});
  }

  AccountMeta account_meta;
  auto account_birthday = ReadUint32(resolve_account_statement, 0);
  auto latest_scanned_block = ReadUint32(resolve_account_statement, 1);
  if (!account_birthday || !latest_scanned_block) {
    return base::unexpected(Error{kInternalError, "Database format error"});
  }

  account_meta.account_birthday = *account_birthday;
  account_meta.latest_scanned_block_id = *latest_scanned_block;
  account_meta.latest_scanned_block_hash =
      resolve_account_statement.ColumnString(2);
  return account_meta;
}

std::optional<ZCashOrchardStorage::Error> ZCashOrchardStorage::HandleChainReorg(
    mojom::AccountIdPtr account_id,
    uint32_t reorg_block_id,
    const std::string& reorg_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return Error{ErrorCode::kDbInitError, database_.GetErrorMessage()};
  }

  sql::Transaction transaction(&database_);

  if (!transaction.Begin()) {
    NOTREACHED_IN_MIGRATION();
    return Error{ErrorCode::kInternalError, database_.GetErrorMessage()};
  }

  sql::Statement remove_from_spent_notes(database_.GetCachedStatement(
      SQL_FROM_HERE, "DELETE FROM " kSpentNotesTable " "
                     "WHERE spent_block_id > ? AND account_id = ?;"));

  remove_from_spent_notes.BindInt64(0, reorg_block_id);
  remove_from_spent_notes.BindString(1, account_id->unique_key);

  sql::Statement remove_from_notes(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "DELETE FROM " kNotesTable " WHERE block_id > ? AND account_id = ?;"));

  remove_from_notes.BindInt64(0, reorg_block_id);
  remove_from_notes.BindString(1, account_id->unique_key);

  sql::Statement update_account_meta(database_.GetCachedStatement(
      SQL_FROM_HERE,

      "UPDATE " kAccountMeta " "
      "SET latest_scanned_block = ?, latest_scanned_block_hash = ? "
      "WHERE account_id = ?;"));

  update_account_meta.BindInt64(0, reorg_block_id);
  update_account_meta.BindString(1, reorg_block_hash);
  update_account_meta.BindString(2, account_id->unique_key);

  if (!remove_from_notes.Run() || !remove_from_spent_notes.Run() ||
      !update_account_meta.Run()) {
    NOTREACHED_IN_MIGRATION();
    transaction.Rollback();
    return Error{ErrorCode::kFailedToExecuteStatement,
                 database_.GetErrorMessage()};
  }

  if (!transaction.Commit()) {
    return Error{ErrorCode::kFailedToExecuteStatement,
                 database_.GetErrorMessage()};
  }

  return std::nullopt;
}

base::expected<std::vector<OrchardNullifier>, ZCashOrchardStorage::Error>
ZCashOrchardStorage::GetNullifiers(mojom::AccountIdPtr account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  sql::Statement resolve_note_spents(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT spent_block_id, nullifier "
      "FROM " kSpentNotesTable " WHERE spent_notes.account_id = ?;"));

  resolve_note_spents.BindString(0, account_id->unique_key);

  std::vector<OrchardNullifier> result;
  while (resolve_note_spents.Step()) {
    OrchardNullifier nf;
    auto block_id = ReadUint32(resolve_note_spents, 0);
    if (!block_id) {
      NOTREACHED_IN_MIGRATION();
      return base::unexpected(
          Error{ErrorCode::kDbInitError, "Wrong database format"});
    }
    nf.block_id = block_id.value();
    auto nullifier = resolve_note_spents.ColumnBlob(1);
    std::copy(nullifier.begin(), nullifier.end(), nf.nullifier.begin());
    result.push_back(std::move(nf));
  }
  if (!resolve_note_spents.Succeeded()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }
  return result;
}

base::expected<std::vector<OrchardNote>, ZCashOrchardStorage::Error>
ZCashOrchardStorage::GetSpendableNotes(mojom::AccountIdPtr account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  sql::Statement resolve_unspent_notes(database_.GetCachedStatement(
      SQL_FROM_HERE,

      "SELECT "
      "notes.block_id, notes.amount,"
      "notes.nullifier FROM " kNotesTable
      " "
      "LEFT OUTER JOIN spent_notes "
      "ON notes.nullifier = spent_notes.nullifier AND notes.account_id = "
      "spent_notes.account_id "
      "WHERE spent_notes.nullifier IS NULL AND notes.account_id = ?;"));

  resolve_unspent_notes.BindString(0, account_id->unique_key);

  std::vector<OrchardNote> result;
  while (resolve_unspent_notes.Step()) {
    OrchardNote note;
    auto block_id = ReadUint32(resolve_unspent_notes, 0);
    auto amount = ReadUint32(resolve_unspent_notes, 1);
    if (!block_id || !amount) {
      NOTREACHED_IN_MIGRATION();
      return base::unexpected(
          Error{ErrorCode::kDbInitError, "Wrong database format"});
    }
    note.block_id = block_id.value();
    note.amount = amount.value();
    auto nullifier = resolve_unspent_notes.ColumnBlob(2);
    std::copy(nullifier.begin(), nullifier.end(), note.nullifier.begin());
    result.push_back(std::move(note));
  }
  return result;
}

std::optional<ZCashOrchardStorage::Error> ZCashOrchardStorage::UpdateNotes(
    mojom::AccountIdPtr account_id,
    const std::vector<OrchardNote>& found_notes,
    const std::vector<OrchardNullifier>& spent_notes,
    const uint32_t latest_scanned_block,
    const std::string& latest_scanned_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return Error{ErrorCode::kDbInitError, database_.GetErrorMessage()};
  }

  sql::Transaction transaction(&database_);
  if (!transaction.Begin()) {
    NOTREACHED_IN_MIGRATION();
    return Error{ErrorCode::kDbInitError, database_.GetErrorMessage()};
  }

  // Insert found notes to the notes table
  sql::Statement statement_populate_notes(database_.GetCachedStatement(
      SQL_FROM_HERE, "INSERT INTO " kNotesTable " "
                     "(account_id, amount, block_id, nullifier) "
                     "VALUES (?, ?, ?, ?);"));

  for (const auto& note : found_notes) {
    statement_populate_notes.Reset(true);
    statement_populate_notes.BindString(0, account_id->unique_key);
    statement_populate_notes.BindInt64(1, note.amount);
    statement_populate_notes.BindInt64(2, note.block_id);
    statement_populate_notes.BindBlob(3, note.nullifier);
    if (!statement_populate_notes.Run()) {
      NOTREACHED_IN_MIGRATION();
      transaction.Rollback();
      return Error{ErrorCode::kFailedToExecuteStatement,
                   database_.GetErrorMessage()};
    }
  }

  // Insert found spent nullifiers to spent notes table
  sql::Statement statement_populate_spent_notes(database_.GetCachedStatement(
      SQL_FROM_HERE, "INSERT INTO " kSpentNotesTable " "
                     "(account_id, spent_block_id, nullifier) "
                     "VALUES (?, ?, ?);"));

  for (const auto& spent : spent_notes) {
    statement_populate_spent_notes.Reset(true);
    statement_populate_spent_notes.BindString(0, account_id->unique_key);
    statement_populate_spent_notes.BindInt64(1, spent.block_id);
    statement_populate_spent_notes.BindBlob(2, spent.nullifier);
    if (!statement_populate_spent_notes.Run()) {
      transaction.Rollback();
      NOTREACHED_IN_MIGRATION();
      return Error{ErrorCode::kFailedToExecuteStatement,
                   database_.GetErrorMessage()};
    }
  }

  // Update account meta
  sql::Statement statement_update_account_meta(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "UPDATE " kAccountMeta " "
      "SET latest_scanned_block = ?, latest_scanned_block_hash = ? "
      "WHERE account_id = ?;"));

  statement_update_account_meta.BindInt64(0, latest_scanned_block);
  statement_update_account_meta.BindString(1, latest_scanned_block_hash);
  statement_update_account_meta.BindString(2, account_id->unique_key);
  if (!statement_update_account_meta.Run()) {
    transaction.Rollback();
    NOTREACHED_IN_MIGRATION();
    return Error{ErrorCode::kFailedToExecuteStatement,
                 database_.GetErrorMessage()};
  }

  if (!transaction.Commit()) {
    return Error{ErrorCode::kFailedToExecuteStatement,
                 database_.GetErrorMessage()};
  }

  return std::nullopt;
}

base::expected<uint32_t, ZCashOrchardStorage::Error>
ZCashOrchardStorage::GetLatestShardIndex(mojom::AccountIdPtr account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  sql::Statement resolve_max_shard_id(
      database_.GetCachedStatement(SQL_FROM_HERE,
                                   "SELECT "
                                   "max(shard_index) FROM " kShardTree " "
                                   "WHERE account_id = ?;"));

  resolve_max_shard_id.BindString(0, account_id->unique_key);
  if (resolve_max_shard_id.Step()) {
    auto shard_index = ReadUint32(resolve_max_shard_id, 0);
    if (!shard_index) {
      NOTREACHED_IN_MIGRATION();
      return base::unexpected(
          Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
    }
    return shard_index.value();
  }

  return 0;
}

std::optional<ZCashOrchardStorage::Error> ZCashOrchardStorage::PutShardRoots(
    mojom::AccountIdPtr account_id,
    uint8_t shard_roots_height,
    uint32_t start_position,
    std::vector<zcash::mojom::SubtreeRootPtr> roots) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return Error{ErrorCode::kDbInitError, database_.GetErrorMessage()};
  }

  sql::Transaction transaction(&database_);
  if (!transaction.Begin()) {
    NOTREACHED_IN_MIGRATION();
    return Error{ErrorCode::kDbInitError, database_.GetErrorMessage()};
  }

  // Insert found notes to the notes table
  sql::Statement statement_populate_roots(database_.GetCachedStatement(
      SQL_FROM_HERE, "INSERT INTO " kShardTree " "
                     "(shard_index, subtree_end_height, root_hash, shard_data, "
                     "contains_marked) "
                     "VALUES (?, ?, ?, ?, ?);"));

  uint32_t counter = start_position;
  for (const auto& root : roots) {
    statement_populate_roots.Reset(true);
    statement_populate_roots.BindInt64(0, counter++);
    statement_populate_roots.BindInt64(1, shard_roots_height);
    statement_populate_roots.BindBlob(2, root->root_hash);
    // statement_populate_roots.BindBlob(3, root->);
    statement_populate_roots.BindInt64(4, 0);
    if (!statement_populate_roots.Run()) {
      NOTREACHED_IN_MIGRATION();
      transaction.Rollback();
      return Error{ErrorCode::kFailedToExecuteStatement,
                   database_.GetErrorMessage()};
    }
  }

  if (!transaction.Commit()) {
    return Error{ErrorCode::kFailedToExecuteStatement,
                 database_.GetErrorMessage()};
  }

  return std::nullopt;
}

std::optional<ZCashOrchardStorage::Error> ZCashOrchardStorage::PutShard(
    mojom::AccountIdPtr account_id,
    OrchardShard shard) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return Error{ErrorCode::kDbInitError, database_.GetErrorMessage()};
  }

  sql::Transaction transaction(&database_);
  if (!transaction.Begin()) {
    NOTREACHED_IN_MIGRATION();
    return Error{ErrorCode::kDbInitError, database_.GetErrorMessage()};
  }

  // Insert found notes to the notes table
  sql::Statement statement_put_shard(database_.GetCachedStatement(
      SQL_FROM_HERE, "INSERT INTO " kShardTree " "
                     "(shard_index, root_hash, shard_data) "
                     "VALUES (:shard_index, :root_hash, :shard_data) "
                     "ON CONFLICT (shard_index) DO UPDATE "
                     "SET root_hash = :root_hash, "
                     "shard_data = :shard_data"));

  statement_put_shard.BindInt64(0, shard.shard_index);
  statement_put_shard.BindBlob(1, shard.root_hash);
  statement_put_shard.BindBlob(2, shard.shard_data);

  if (!statement_put_shard.Run()) {
    NOTREACHED_IN_MIGRATION();
    transaction.Rollback();
    return Error{ErrorCode::kFailedToExecuteStatement,
                 database_.GetErrorMessage()};
  }

  if (!transaction.Commit()) {
    return Error{ErrorCode::kFailedToExecuteStatement,
                 database_.GetErrorMessage()};
  }

  return std::nullopt;
}

base::expected<OrchardShard, ZCashOrchardStorage::Error>
ZCashOrchardStorage::LastShard(mojom::AccountIdPtr account_id, uint8_t shard_height) {
  auto shard_index = GetLatestShardIndex(account_id.Clone());
  if (!shard_index.has_value()) {
    return base::unexpected(shard_index.error());
  }

  return GetShard(account_id.Clone(), OrchardShardAddress { shard_height, shard_index.value() });
}

base::expected<std::vector<OrchardShardAddress>, ZCashOrchardStorage::Error>
ZCashOrchardStorage::GetShardRoots(mojom::AccountIdPtr account_id,
                                   uint8_t shard_level) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  std::vector<OrchardShardAddress> result;

  sql::Statement resolve_shards_statement(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT shard_index FROM " kShardTree " ORDER BY shard_index"));

  while (!resolve_shards_statement.Step()) {
    auto shard_index = ReadUint32(resolve_shards_statement, 0);
    if (!shard_index) {
      NOTREACHED_IN_MIGRATION();
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement, ""});
    }
    result.push_back(OrchardShardAddress{shard_level, shard_index.value()});
  }

  return result;
}

base::expected<uint32_t, ZCashOrchardStorage::Error>
ZCashOrchardStorage::CheckpointCount(mojom::AccountIdPtr account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  sql::Statement resolve_checkpoints_count(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT COUNT(*) FROM " kShardTreeCheckpoints));

  if (!resolve_checkpoints_count.Step()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kNoCheckpoints, database_.GetErrorMessage()});
  }

  auto value = ReadUint32(resolve_checkpoints_count, 0);

  if (!value) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kNoCheckpoints, database_.GetErrorMessage()});
  }

  return *value;
}

base::expected<uint32_t, ZCashOrchardStorage::Error>
ZCashOrchardStorage::MinCheckpointId(mojom::AccountIdPtr account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  sql::Statement resolve_min_checkpoint_id(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT MIN(checkpoint_id) " kShardTreeCheckpoints));

  if (!resolve_min_checkpoint_id.Step()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kNoCheckpoints, database_.GetErrorMessage()});
  }

  auto value = ReadUint32(resolve_min_checkpoint_id, 0);

  if (!value) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kNoCheckpoints, database_.GetErrorMessage()});
  }

  return *value;
}

base::expected<uint32_t, ZCashOrchardStorage::Error>
ZCashOrchardStorage::MaxCheckpointId(mojom::AccountIdPtr account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  sql::Statement resolve_max_checkpoint_id(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT MAX(checkpoint_id) " kShardTreeCheckpoints));

  if (!resolve_max_checkpoint_id.Step()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kNoCheckpoints, database_.GetErrorMessage()});
  }

  auto value = ReadUint32(resolve_max_checkpoint_id, 0);

  if (!value) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kNoCheckpoints, database_.GetErrorMessage()});
  }

  return *value;
}

base::expected<uint32_t, ZCashOrchardStorage::Error> ZCashOrchardStorage::GetCheckpointAtDepth(
    mojom::AccountIdPtr account_id, uint32_t depth) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  sql::Statement get_checkpoint_at_depth_statement(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT checkpoint_id, position "
      "FROM " kShardTreeCheckpoints " "
      "ORDER BY checkpoint_id DESC "
      "LIMIT 1 "
      "OFFSET ?"));

  get_checkpoint_at_depth_statement.BindInt64(0, depth);

  if (!get_checkpoint_at_depth_statement.Step()) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kNoCheckpoints, database_.GetErrorMessage()});
  }

  auto value = ReadUint32(get_checkpoint_at_depth_statement, 0);

  if (!value) {
    NOTREACHED_IN_MIGRATION();
    return base::unexpected(
        Error{ErrorCode::kNoCheckpoints, database_.GetErrorMessage()});
  }

  return *value;
}

}  // namespace brave_wallet
