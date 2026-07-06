/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_storage.h"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/files/file_util.h"
#include "base/notreached.h"
#include "base/sequence_checker.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/types/expected_macros.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_wallet {

namespace {

constexpr int kEmptyDbVersionNumber = 1;
constexpr int kCurrentVersionNumber = 3;

// CREATE TABLE templates. %s is replaced with the pool-specific table name.
// For checkpoints_mark_removed, the first %s is the marks table and the second
// %s is the referenced checkpoints table.
constexpr char kCreateNotesSql[] =
    "CREATE TABLE %s ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "account_id TEXT NOT NULL,"
    "amount INTEGER NOT NULL,"
    "addr BLOB NOT NULL,"
    "block_id INTEGER NOT NULL,"
    "commitment_tree_position INTEGER,"
    "nullifier BLOB NOT NULL UNIQUE,"
    "rho BLOB NOT NULL,"
    "rseed BLOB NOT NULL);";

constexpr char kCreateSpentNotesSql[] =
    "CREATE TABLE %s ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "account_id TEXT NOT NULL,"
    "spent_block_id INTEGER NOT NULL,"
    "nullifier BLOB NOT NULL UNIQUE);";

constexpr char kCreateAccountMetaSql[] =
    "CREATE TABLE account_meta ("
    "account_id TEXT NOT NULL PRIMARY KEY,"
    "account_birthday INTEGER NOT NULL,"
    "latest_scanned_block INTEGER,"
    "latest_scanned_block_hash TEXT);";

constexpr char kCreateShardTreeSql[] =
    "CREATE TABLE %s ("
    "account_id TEXT NOT NULL,"
    "shard_index INTEGER NOT NULL,"
    "subtree_end_height INTEGER,"
    "root_hash BLOB,"
    "shard_data BLOB,"
    "CONSTRAINT shard_index_unique UNIQUE (shard_index, account_id),"
    "CONSTRAINT root_unique UNIQUE (root_hash, account_id));";

constexpr char kCreateShardTreeCheckpointsSql[] =
    "CREATE TABLE %s ("
    "account_id TEXT NOT NULL,"
    "checkpoint_id INTEGER NOT NULL,"
    "position INTEGER,"
    "PRIMARY KEY (account_id, checkpoint_id));";

constexpr char kCreateCheckpointsMarksRemovedSql[] =
    "CREATE TABLE %s ("
    "account_id TEXT NOT NULL,"
    "checkpoint_id INTEGER NOT NULL,"
    "mark_removed_position INTEGER NOT NULL,"
    "FOREIGN KEY (account_id, checkpoint_id) REFERENCES %s(account_id, "
    "checkpoint_id) ON DELETE CASCADE,"
    "CONSTRAINT spend_position_unique UNIQUE "
    "(checkpoint_id, mark_removed_position, account_id));";

constexpr char kCreateShardTreeCapSql[] =
    "CREATE TABLE %s ("
    "account_id TEXT NOT NULL,"
    "cap_data BLOB NOT NULL,"
    "CONSTRAINT shard_tree_cap_unique UNIQUE (account_id));";

struct PoolTableNames {
  std::string notes;
  std::string spent_notes;
  std::string shard_tree;
  std::string shard_tree_cap;
  std::string checkpoints;
  std::string checkpoints_mark_removed;
};

PoolTableNames TablesForPool(OrchardPool pool) {
  switch (pool) {
    case OrchardPool::kOrchard:
      return PoolTableNames{"notes",       "spent_notes",
                            "shard_tree",  "shard_tree_cap",
                            "checkpoints", "checkpoints_mark_removed"};
    case OrchardPool::kIronwood:
      return PoolTableNames{
          "notes_ironwood",       "spent_notes_ironwood",
          "shard_tree_ironwood",  "shard_tree_cap_ironwood",
          "checkpoints_ironwood", "checkpoints_mark_removed_ironwood"};
  }
  NOTREACHED();
}

bool CreatePoolDataTables(sql::Database& database, const PoolTableNames& t) {
  return database.Execute(absl::StrFormat(kCreateNotesSql, t.notes.c_str())) &&
         database.Execute(
             absl::StrFormat(kCreateSpentNotesSql, t.spent_notes.c_str())) &&
         database.Execute(
             absl::StrFormat(kCreateShardTreeSql, t.shard_tree.c_str())) &&
         database.Execute(absl::StrFormat(kCreateShardTreeCheckpointsSql,
                                          t.checkpoints.c_str())) &&
         database.Execute(absl::StrFormat(kCreateCheckpointsMarksRemovedSql,
                                          t.checkpoints_mark_removed.c_str(),
                                          t.checkpoints.c_str())) &&
         database.Execute(
             absl::StrFormat(kCreateShardTreeCapSql, t.shard_tree_cap.c_str()));
}

sql::Statement PrepareStatement(sql::Database& database,
                                const std::string& sql) {
  return sql::Statement(database.GetUniqueStatement(sql));
}

// Rebuilds the v2 orchard "checkpoints" and "checkpoints_mark_removed"
// tables in place so they use the fixed v3 schema:
// - checkpoints v2 had `checkpoint_id` as a standalone PRIMARY KEY, making
//   checkpoint ids unique across *all* accounts instead of per-account.
// - checkpoints_mark_removed v2 had a FOREIGN KEY referencing a nonexistent
//   "orchard_tree_checkpoints" table, so the constraint never applied.
// SQLite can't alter a PRIMARY KEY/FOREIGN KEY in place, so the affected
// tables are renamed out of the way, recreated with the fixed schema,
// repopulated from the renamed originals, and the originals are dropped.
bool MigrateOrchardCheckpointsTables(sql::Database& database) {
  const PoolTableNames tables = TablesForPool(OrchardPool::kOrchard);

  return database.Execute(absl::StrFormat(
             "ALTER TABLE %s RENAME TO checkpoints_mark_removed_v2_old;",
             tables.checkpoints_mark_removed.c_str())) &&
         database.Execute(
             absl::StrFormat("ALTER TABLE %s RENAME TO checkpoints_v2_old;",
                             tables.checkpoints.c_str())) &&
         database.Execute(absl::StrFormat(kCreateShardTreeCheckpointsSql,
                                          tables.checkpoints.c_str())) &&
         database.Execute(absl::StrFormat(
             "INSERT INTO %s (account_id, checkpoint_id, position) "
             "SELECT account_id, checkpoint_id, position FROM "
             "checkpoints_v2_old;",
             tables.checkpoints.c_str())) &&
         database.Execute("DROP TABLE checkpoints_v2_old;") &&
         database.Execute(
             absl::StrFormat(kCreateCheckpointsMarksRemovedSql,
                             tables.checkpoints_mark_removed.c_str(),
                             tables.checkpoints.c_str())) &&
         database.Execute(absl::StrFormat(
             "INSERT INTO %s (account_id, checkpoint_id, "
             "mark_removed_position) SELECT account_id, checkpoint_id, "
             "mark_removed_position FROM checkpoints_mark_removed_v2_old;",
             tables.checkpoints_mark_removed.c_str())) &&
         database.Execute("DROP TABLE checkpoints_mark_removed_v2_old;");
}

template <size_t const T>
base::expected<std::optional<std::array<uint8_t, T>>, std::string>
ReadSizedBlob(sql::Statement& statement, int position) {
  int columns = statement.ColumnCount();
  if (position >= columns) {
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

std::optional<uint32_t> ReadUint32(sql::Statement& statement, int position) {
  auto v = statement.ColumnInt64(position);
  auto checked_v = base::CheckedNumeric<uint32_t>(v);
  if (!checked_v.IsValid()) {
    return std::nullopt;
  }
  return checked_v.ValueOrDie();
}

// SQLite doesn't support uint64 natively so we restore it from int64 bytes.
// We also check that value matches int64 positive range.
std::optional<uint64_t> ReadAmount(sql::Statement& statement, int position) {
  auto v = statement.ColumnInt64(position);
  auto checked_v = base::CheckedNumeric<uint64_t>(v);
  if (!checked_v.IsValid()) {
    return std::nullopt;
  }
  return checked_v.ValueOrDie();
}

std::optional<int64_t> AmountToInt64(uint64_t value) {
  auto checked_v = base::CheckedNumeric<int64_t>(value);
  if (!checked_v.IsValid()) {
    return std::nullopt;
  }
  return checked_v.ValueOrDie();
}

base::expected<CheckpointTreeState, std::string> ReadCheckpointTreeState(
    sql::Statement& statement,
    int position) {
  if (statement.GetColumnType(position) == sql::ColumnType::kNull) {
    return std::nullopt;
  }
  auto v = ReadUint32(statement, position);
  if (!v) {
    return base::unexpected("Format error");
  }
  return *v;
}

base::expected<std::optional<OrchardShardRootHash>, std::string> ReadRootHash(
    sql::Statement& statement,
    int position) {
  if (statement.GetColumnType(position) == sql::ColumnType::kNull) {
    return std::nullopt;
  }
  auto v = statement.ColumnBlob(position);
  if (v.size() != kOrchardShardTreeHashSize) {
    return base::unexpected("Size error");
  }
  std::array<uint8_t, kOrchardShardTreeHashSize> result;
  base::span(result).copy_from(v);
  return result;
}

}  // namespace

OrchardStorage::TransactionScope::TransactionScope(sql::Database& database)
    : database_(database) {
  transaction_ = std::make_unique<sql::Transaction>(&database);
}
OrchardStorage::TransactionScope::~TransactionScope() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}
OrchardStorage::TransactionScope::TransactionScope(
    OrchardStorage::TransactionScope&&) = default;
OrchardStorage::TransactionScope& OrchardStorage::TransactionScope::operator=(
    OrchardStorage::TransactionScope&&) = default;
base::expected<void, OrchardStorage::Error>
OrchardStorage::TransactionScope::Begin(
    base::PassKey<class OrchardStorage> pass_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!transaction_->Begin()) {
    return base::unexpected(Error{ErrorCode::kFailedToCreateTransaction,
                                  database_->GetErrorMessage()});
  }
  return base::ok();
}
base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::TransactionScope::Commit() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!transaction_->Commit()) {
    return base::unexpected(Error{ErrorCode::kFailedToCommitTransaction,
                                  database_->GetErrorMessage()});
  }
  return base::ok(Result::kSuccess);
}

OrchardStorage::AccountMeta::AccountMeta() = default;
OrchardStorage::AccountMeta::~AccountMeta() = default;
OrchardStorage::AccountMeta::AccountMeta(const AccountMeta&) = default;
OrchardStorage::AccountMeta& OrchardStorage::AccountMeta::operator=(
    const AccountMeta&) = default;

OrchardStorage::OrchardStorage(const base::FilePath& path_to_database)
    : db_file_path_(path_to_database),
      database_(sql::Database::Tag("OrchardStorageDatabase")) {
  // Instantiated on default sequence. Operates on dedicated sequence.
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

OrchardStorage::~OrchardStorage() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

base::expected<OrchardStorage::TransactionScope, OrchardStorage::Error>
OrchardStorage::Transactionally() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  TransactionScope tx(database_);
  auto begin_result = tx.Begin(base::PassKey<class OrchardStorage>());
  if (!begin_result.has_value()) {
    return base::unexpected(begin_result.error());
  }
  return base::ok(std::move(tx));
}

bool OrchardStorage::EnsureDbInit() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (database_.is_open()) {
    return true;
  }
  return CreateOrUpdateDatabase();
}

void OrchardStorage::ResetDatabase() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  database_.Close();
  sql::Database::Delete(db_file_path_);
}

bool OrchardStorage::CreateOrUpdateDatabase() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const base::FilePath dir = db_file_path_.DirName();
  if (!base::DirectoryExists(dir) && !base::CreateDirectory(dir)) {
    return false;
  }

  if (!database_.Open(db_file_path_)) {
    return false;
  }

  sql::MetaTable meta_table;
  if (!meta_table.Init(&database_, kEmptyDbVersionNumber,
                       kEmptyDbVersionNumber)) {
    database_.Close();
    return false;
  }

  if (meta_table.GetVersionNumber() == kEmptyDbVersionNumber) {
    if (!CreateSchema() ||
        !meta_table.SetVersionNumber(kCurrentVersionNumber)) {
      database_.Close();
      return false;
    }
  } else if (meta_table.GetVersionNumber() < kCurrentVersionNumber) {
    if (!UpdateSchema() ||
        !meta_table.SetVersionNumber(kCurrentVersionNumber)) {
      database_.Close();
      return false;
    }
  }

  return true;
}

bool OrchardStorage::CreateSchema() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);

  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sql::Transaction transaction(&database_);
  return transaction.Begin() && database_.Execute(kCreateAccountMetaSql) &&
         CreatePoolDataTables(database_,
                              TablesForPool(OrchardPool::kOrchard)) &&
         CreatePoolDataTables(database_,
                              TablesForPool(OrchardPool::kIronwood)) &&
         transaction.Commit();
}

bool OrchardStorage::UpdateSchema() {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sql::MetaTable meta_table;
  if (!meta_table.Init(&database_, kEmptyDbVersionNumber,
                       kEmptyDbVersionNumber)) {
    return false;
  }
  // Only the v2 → v3 migration is supported. The existing orchard
  // "checkpoints" and "checkpoints_mark_removed" tables are rebuilt in place
  // to use the fixed schema (see MigrateOrchardCheckpointsTables); other
  // orchard tables and account_meta are kept as-is. A parallel set of
  // ironwood tables is created.
  if (meta_table.GetVersionNumber() != 2) {
    return false;
  }

  sql::Transaction transaction(&database_);
  if (!transaction.Begin()) {
    return false;
  }

  if (!MigrateOrchardCheckpointsTables(database_)) {
    return false;
  }

  if (!CreatePoolDataTables(database_, TablesForPool(OrchardPool::kIronwood))) {
    return false;
  }

  return transaction.Commit();
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::RegisterAccount(const mojom::AccountIdPtr& account_id,
                                uint32_t account_birthday_block) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  sql::Statement register_account_statement(
      database_.GetCachedStatement(SQL_FROM_HERE,
                                   "INSERT INTO account_meta "
                                   "(account_id, account_birthday) "
                                   "VALUES (?, ?)"));

  register_account_statement.BindString(0, account_id->unique_key);
  register_account_statement.BindInt64(1, account_birthday_block);

  if (!register_account_statement.Run()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(Result::kSuccess);
}

base::expected<std::optional<OrchardStorage::AccountMeta>,
               OrchardStorage::Error>
OrchardStorage::GetAccountMeta(const mojom::AccountIdPtr& account_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  sql::Statement resolve_account_statement(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT account_birthday, latest_scanned_block, "
      "latest_scanned_block_hash FROM account_meta WHERE account_id = ?;"));

  resolve_account_statement.BindString(0, account_id->unique_key);

  if (!resolve_account_statement.Step()) {
    if (!resolve_account_statement.Succeeded()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    } else {
      return base::ok(std::nullopt);
    }
  }

  AccountMeta account_meta;
  auto account_birthday = ReadUint32(resolve_account_statement, 0);
  if (!account_birthday) {
    return base::unexpected(
        Error{ErrorCode::kConsistencyError, "Wrong account birthday format."});
  }
  account_meta.account_birthday = account_birthday.value();

  if (resolve_account_statement.GetColumnType(1) != sql::ColumnType::kNull) {
    auto latest_scanned_block = ReadUint32(resolve_account_statement, 1);
    if (!latest_scanned_block) {
      return base::unexpected(Error{ErrorCode::kConsistencyError,
                                    "Wrong latest scanned block format"});
    }
    account_meta.latest_scanned_block_id = latest_scanned_block.value();
  }

  if (resolve_account_statement.GetColumnType(2) != sql::ColumnType::kNull) {
    account_meta.latest_scanned_block_hash =
        resolve_account_statement.ColumnString(2);
  }

  return base::ok(account_meta);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::HandleChainReorg(const mojom::AccountIdPtr& account_id,
                                 uint32_t reorg_block_id,
                                 const std::string& reorg_block_hash) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  for (OrchardPool pool : {OrchardPool::kOrchard, OrchardPool::kIronwood}) {
    const PoolTableNames tables = TablesForPool(pool);

    sql::Statement remove_from_spent_notes(PrepareStatement(
        database_,
        absl::StrFormat(
            "DELETE FROM %s WHERE spent_block_id > ? AND account_id = ?;",
            tables.spent_notes.c_str())));
    remove_from_spent_notes.BindInt64(0, reorg_block_id);
    remove_from_spent_notes.BindString(1, account_id->unique_key);

    sql::Statement remove_from_notes(PrepareStatement(
        database_,
        absl::StrFormat("DELETE FROM %s WHERE block_id > ? AND account_id = ?;",
                        tables.notes.c_str())));
    remove_from_notes.BindInt64(0, reorg_block_id);
    remove_from_notes.BindString(1, account_id->unique_key);

    if (!remove_from_notes.Run() || !remove_from_spent_notes.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
  }

  sql::Statement update_account_meta(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "UPDATE account_meta "
      "SET latest_scanned_block = ?, latest_scanned_block_hash = ? "
      "WHERE account_id = ?;"));
  update_account_meta.BindInt64(0, reorg_block_id);
  update_account_meta.BindString(1, reorg_block_hash);
  update_account_meta.BindString(2, account_id->unique_key);

  if (!update_account_meta.Run()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(Result::kSuccess);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::ResetAccountSyncState(
    const mojom::AccountIdPtr& account_id,
    std::optional<uint32_t> account_birthday_block) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  for (OrchardPool pool : {OrchardPool::kOrchard, OrchardPool::kIronwood}) {
    const PoolTableNames tables = TablesForPool(pool);

    sql::Statement clear_cap_statement(PrepareStatement(
        database_, absl::StrFormat("DELETE FROM %s WHERE account_id = ?;",
                                   tables.shard_tree_cap.c_str())));
    clear_cap_statement.BindString(0, account_id->unique_key);

    sql::Statement clear_shards_statement(PrepareStatement(
        database_, absl::StrFormat("DELETE FROM %s WHERE account_id = ?;",
                                   tables.shard_tree.c_str())));
    clear_shards_statement.BindString(0, account_id->unique_key);

    sql::Statement clear_discovered_notes(PrepareStatement(
        database_, absl::StrFormat("DELETE FROM %s WHERE account_id = ?;",
                                   tables.notes.c_str())));
    clear_discovered_notes.BindString(0, account_id->unique_key);

    sql::Statement clear_spent_notes(PrepareStatement(
        database_, absl::StrFormat("DELETE FROM %s WHERE account_id = ?;",
                                   tables.spent_notes.c_str())));
    clear_spent_notes.BindString(0, account_id->unique_key);

    sql::Statement clear_checkpoints_statement(PrepareStatement(
        database_, absl::StrFormat("DELETE FROM %s WHERE account_id = ?;",
                                   tables.checkpoints.c_str())));
    clear_checkpoints_statement.BindString(0, account_id->unique_key);

    if (!clear_cap_statement.Run() || !clear_shards_statement.Run() ||
        !clear_discovered_notes.Run() || !clear_spent_notes.Run() ||
        !clear_checkpoints_statement.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
  }

  if (account_birthday_block) {
    sql::Statement update_account_meta_statement(database_.GetCachedStatement(
        SQL_FROM_HERE,
        "UPDATE account_meta SET latest_scanned_block = NULL, "
        "latest_scanned_block_hash = NULL, account_birthday = ? "
        "WHERE account_id = ?;"));
    update_account_meta_statement.BindInt64(0, *account_birthday_block);
    update_account_meta_statement.BindString(1, account_id->unique_key);
    if (!update_account_meta_statement.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
  } else {
    sql::Statement update_account_meta_statement(database_.GetCachedStatement(
        SQL_FROM_HERE,
        "UPDATE account_meta SET latest_scanned_block = NULL, "
        "latest_scanned_block_hash = NULL WHERE account_id = ?;"));
    update_account_meta_statement.BindString(0, account_id->unique_key);
    if (!update_account_meta_statement.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
  }

  return base::ok(Result::kSuccess);
}

base::expected<std::vector<OrchardNoteSpend>, OrchardStorage::Error>
OrchardStorage::GetNullifiers(OrchardPool pool,
                              const mojom::AccountIdPtr& account_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement resolve_note_spents(PrepareStatement(
      database_,
      absl::StrFormat(
          "SELECT spent_block_id, nullifier FROM %s WHERE account_id = ?;",
          tables.spent_notes.c_str())));
  resolve_note_spents.BindString(0, account_id->unique_key);

  std::vector<OrchardNoteSpend> result;
  while (resolve_note_spents.Step()) {
    OrchardNoteSpend spend;
    auto block_id = ReadUint32(resolve_note_spents, 0);
    if (!block_id) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong block id format"});
    }
    spend.block_id = block_id.value();
    auto nullifier_blob = resolve_note_spents.ColumnBlob(1);
    if (nullifier_blob.size() != kOrchardNullifierSize) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong nullifier size"});
    }
    base::span(spend.nullifier).copy_from(nullifier_blob);
    result.push_back(std::move(spend));
  }
  if (!resolve_note_spents.Succeeded()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }
  return base::ok(std::move(result));
}

base::expected<std::vector<OrchardNote>, OrchardStorage::Error>
OrchardStorage::GetSpendableNotes(OrchardPool pool,
                                  const mojom::AccountIdPtr& account_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement resolve_unspent_notes(PrepareStatement(
      database_,
      absl::StrFormat(
          "SELECT notes.block_id, notes.commitment_tree_position, notes.amount,"
          "notes.rho, notes.rseed, notes.nullifier, notes.addr FROM %s AS "
          "notes "
          "LEFT OUTER JOIN %s AS spent_notes "
          "ON notes.nullifier = spent_notes.nullifier "
          "AND notes.account_id = spent_notes.account_id "
          "WHERE spent_notes.nullifier IS NULL AND notes.account_id = ?;",
          tables.notes.c_str(), tables.spent_notes.c_str())));
  resolve_unspent_notes.BindString(0, account_id->unique_key);

  std::vector<OrchardNote> result;
  while (resolve_unspent_notes.Step()) {
    OrchardNote note;
    auto block_id = ReadUint32(resolve_unspent_notes, 0);
    auto commitment_tree_position = ReadUint32(resolve_unspent_notes, 1);
    auto amount = ReadAmount(resolve_unspent_notes, 2);
    if (!block_id || !amount || !commitment_tree_position) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong database format"});
    }
    auto rho = ReadSizedBlob<kOrchardNoteRhoSize>(resolve_unspent_notes, 3);
    auto rseed = ReadSizedBlob<kOrchardNoteRSeedSize>(resolve_unspent_notes, 4);
    auto nf = ReadSizedBlob<kOrchardNullifierSize>(resolve_unspent_notes, 5);
    auto addr = ReadSizedBlob<kOrchardRawBytesSize>(resolve_unspent_notes, 6);

    if (!rho.has_value() || !rho.value() || !rseed.has_value() ||
        !rseed.value() || !nf.has_value() || !nf.value() || !addr.has_value() ||
        !addr.value()) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong database format"});
    }

    note.block_id = block_id.value();
    note.amount = *amount;
    note.orchard_commitment_tree_position = commitment_tree_position.value();
    note.rho = **rho;
    note.seed = **rseed;
    note.nullifier = **nf;
    note.addr = **addr;
    result.push_back(std::move(note));
  }

  if (!resolve_unspent_notes.Succeeded()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(std::move(result));
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::UpdateNotes(OrchardPool pool,
                            const mojom::AccountIdPtr& account_id,
                            base::span<const OrchardNote> found_notes,
                            base::span<const OrchardNoteSpend> found_nullifiers,
                            const uint32_t latest_scanned_block,
                            const std::string& latest_scanned_block_hash) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  const PoolTableNames tables = TablesForPool(pool);

  sql::Statement statement_populate_notes(PrepareStatement(
      database_,
      absl::StrFormat(
          "INSERT INTO %s "
          "(account_id, amount, block_id, commitment_tree_position, "
          "nullifier, rho, rseed, addr) VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
          tables.notes.c_str())));

  for (const auto& note : found_notes) {
    statement_populate_notes.Reset(true);
    statement_populate_notes.BindString(0, account_id->unique_key);
    auto amount_i64 = AmountToInt64(note.amount);
    if (!amount_i64) {
      return base::unexpected(
          Error{ErrorCode::kFailedToExecuteStatement, "Wrong amount value"});
    }
    statement_populate_notes.BindInt64(1, amount_i64.value());
    statement_populate_notes.BindInt64(2, note.block_id);
    statement_populate_notes.BindInt64(3,
                                       note.orchard_commitment_tree_position);
    statement_populate_notes.BindBlob(4, note.nullifier);
    statement_populate_notes.BindBlob(5, note.rho);
    statement_populate_notes.BindBlob(6, note.seed);
    statement_populate_notes.BindBlob(7, note.addr);

    if (!statement_populate_notes.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
  }

  sql::Statement statement_populate_spent_notes(PrepareStatement(
      database_,
      absl::StrFormat("INSERT INTO %s (account_id, spent_block_id, nullifier) "
                      "VALUES (?, ?, ?);",
                      tables.spent_notes.c_str())));

  for (const auto& spent : found_nullifiers) {
    statement_populate_spent_notes.Reset(true);
    statement_populate_spent_notes.BindString(0, account_id->unique_key);
    statement_populate_spent_notes.BindInt64(1, spent.block_id);
    statement_populate_spent_notes.BindBlob(2, spent.nullifier);
    if (!statement_populate_spent_notes.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
  }

  sql::Statement statement_update_account_meta(database_.GetCachedStatement(
      SQL_FROM_HERE,
      "UPDATE account_meta "
      "SET latest_scanned_block = ?, latest_scanned_block_hash = ? "
      "WHERE account_id = ?;"));
  statement_update_account_meta.BindInt64(0, latest_scanned_block);
  statement_update_account_meta.BindString(1, latest_scanned_block_hash);
  statement_update_account_meta.BindString(2, account_id->unique_key);
  if (!statement_update_account_meta.Run()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(Result::kSuccess);
}

base::expected<std::optional<uint32_t>, OrchardStorage::Error>
OrchardStorage::GetLatestShardIndex(OrchardPool pool,
                                    const mojom::AccountIdPtr& account_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement resolve_max_shard_id(PrepareStatement(
      database_,
      absl::StrFormat("SELECT MAX(shard_index) FROM %s WHERE account_id = ?;",
                      tables.shard_tree.c_str())));
  resolve_max_shard_id.BindString(0, account_id->unique_key);
  if (!resolve_max_shard_id.Step()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  if (resolve_max_shard_id.GetColumnType(0) == sql::ColumnType::kNull) {
    return base::ok(std::nullopt);
  }

  auto shard_index = ReadUint32(resolve_max_shard_id, 0);
  if (!shard_index) {
    return base::unexpected(
        Error{ErrorCode::kConsistencyError, "Wrong shard index format"});
  }
  return base::ok(shard_index.value());
}

base::expected<std::optional<OrchardShardTreeCap>, OrchardStorage::Error>
OrchardStorage::GetCap(OrchardPool pool,
                       const mojom::AccountIdPtr& account_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement resolve_cap(PrepareStatement(
      database_,
      absl::StrFormat("SELECT cap_data FROM %s WHERE account_id = ?;",
                      tables.shard_tree_cap.c_str())));
  resolve_cap.BindString(0, account_id->unique_key);

  if (!resolve_cap.Step()) {
    if (!resolve_cap.Succeeded()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
    return base::ok(std::nullopt);
  }

  return base::ok(base::ToVector(resolve_cap.ColumnBlob(0)));
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::PutCap(OrchardPool pool,
                       const mojom::AccountIdPtr& account_id,
                       const OrchardShardTreeCap& cap) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  auto existing_cap = GetCap(pool, account_id);
  RETURN_IF_ERROR(existing_cap);

  const PoolTableNames tables = TablesForPool(pool);
  if (!existing_cap.value()) {
    sql::Statement stmnt(PrepareStatement(
        database_,
        absl::StrFormat("INSERT INTO %s (account_id, cap_data) VALUES (?, ?);",
                        tables.shard_tree_cap.c_str())));
    stmnt.BindString(0, account_id->unique_key);
    stmnt.BindBlob(1, cap);
    if (!stmnt.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
  } else {
    sql::Statement stmnt(PrepareStatement(
        database_,
        absl::StrFormat("UPDATE %s SET cap_data = ? WHERE account_id = ?;",
                        tables.shard_tree_cap.c_str())));
    stmnt.BindBlob(0, cap);
    stmnt.BindString(1, account_id->unique_key);
    if (!stmnt.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
  }

  return base::ok(Result::kSuccess);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::UpdateSubtreeRoots(
    OrchardPool pool,
    const mojom::AccountIdPtr& account_id,
    uint32_t start_index,
    const std::vector<zcash::mojom::SubtreeRootPtr>& roots) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement statement_populate_roots(PrepareStatement(
      database_, absl::StrFormat("INSERT INTO %s "
                                 "(shard_index, subtree_end_height, "
                                 "root_hash, shard_data, account_id) "
                                 "VALUES (?, ?, ?, ?, ?);",
                                 tables.shard_tree.c_str())));

  sql::Statement statement_update_roots(PrepareStatement(
      database_,
      absl::StrFormat("UPDATE %s SET subtree_end_height = ?, root_hash = ? "
                      "WHERE shard_index = ? AND account_id = ?;",
                      tables.shard_tree.c_str())));

  for (size_t i = 0; i < roots.size(); i++) {
    if (!roots[i] ||
        roots[i]->complete_block_hash.size() != kOrchardCompleteBlockHashSize) {
      return base::unexpected(
          Error{ErrorCode::kInternalError, "Complete block hash differs"});
    }

    statement_populate_roots.Reset(true);
    statement_populate_roots.BindInt64(0, start_index + i);
    statement_populate_roots.BindInt64(1, roots[i]->complete_block_height);
    statement_populate_roots.BindBlob(2, roots[i]->complete_block_hash);
    statement_populate_roots.BindNull(3);
    statement_populate_roots.BindString(4, account_id->unique_key);
    if (!statement_populate_roots.Run()) {
      if (database_.GetErrorCode() == 19 /*SQLITE_CONSTRAINT*/) {
        statement_update_roots.Reset(true);
        statement_update_roots.BindInt64(0, roots[i]->complete_block_height);
        statement_update_roots.BindBlob(1, roots[i]->complete_block_hash);
        statement_update_roots.BindInt64(2, start_index + i);
        statement_update_roots.BindString(3, account_id->unique_key);
        if (!statement_update_roots.Run()) {
          return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                        database_.GetErrorMessage()});
        }
      } else {
        return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                      database_.GetErrorMessage()});
      }
    }
  }

  return base::ok(Result::kSuccess);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::TruncateShards(OrchardPool pool,
                               const mojom::AccountIdPtr& account_id,
                               uint32_t shard_index) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement remove_shard_by_id(PrepareStatement(
      database_,
      absl::StrFormat(
          "DELETE FROM %s WHERE shard_index >= ? AND account_id = ?;",
          tables.shard_tree.c_str())));
  remove_shard_by_id.BindInt64(0, shard_index);
  remove_shard_by_id.BindString(1, account_id->unique_key);

  if (!remove_shard_by_id.Run()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(Result::kSuccess);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::PutShard(OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardShard& shard) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  ASSIGN_OR_RETURN(auto existing_shard,
                   GetShard(pool, account_id, shard.address));

  const PoolTableNames tables = TablesForPool(pool);
  if (existing_shard.has_value()) {
    sql::Statement statement_update_shard(PrepareStatement(
        database_,
        absl::StrFormat("UPDATE %s SET root_hash = ?, shard_data = ? "
                        "WHERE shard_index = ? AND account_id = ?;",
                        tables.shard_tree.c_str())));
    if (!shard.root_hash) {
      statement_update_shard.BindNull(0);
    } else {
      statement_update_shard.BindBlob(0, shard.root_hash.value());
    }
    statement_update_shard.BindBlob(1, shard.shard_data);
    statement_update_shard.BindInt64(2, shard.address.index);
    statement_update_shard.BindString(3, account_id->unique_key);

    if (!statement_update_shard.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
  } else {
    sql::Statement statement_put_shard(PrepareStatement(
        database_,
        absl::StrFormat(
            "INSERT INTO %s (shard_index, root_hash, shard_data, account_id) "
            "VALUES (?, ?, ?, ?);",
            tables.shard_tree.c_str())));
    statement_put_shard.BindInt64(0, shard.address.index);
    if (!shard.root_hash) {
      statement_put_shard.BindNull(1);
    } else {
      statement_put_shard.BindBlob(1, shard.root_hash.value());
    }
    statement_put_shard.BindBlob(2, shard.shard_data);
    statement_put_shard.BindString(3, account_id->unique_key);

    if (!statement_put_shard.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
  }

  return base::ok(Result::kSuccess);
}

base::expected<std::optional<OrchardShard>, OrchardStorage::Error>
OrchardStorage::GetShard(OrchardPool pool,
                         const mojom::AccountIdPtr& account_id,
                         const OrchardShardAddress& address) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement resolve_shard_statement(PrepareStatement(
      database_, absl::StrFormat("SELECT root_hash, shard_data FROM %s "
                                 "WHERE account_id = ? AND shard_index = ?;",
                                 tables.shard_tree.c_str())));
  resolve_shard_statement.BindString(0, account_id->unique_key);
  resolve_shard_statement.BindInt64(1, address.index);

  if (resolve_shard_statement.Step()) {
    auto hash = ReadRootHash(resolve_shard_statement, 0);
    if (!hash.has_value()) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong root hash format"});
    }
    return base::ok(
        OrchardShard(address, hash.value(),
                     base::ToVector(resolve_shard_statement.ColumnBlob(1))));
  }

  if (!resolve_shard_statement.Succeeded()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(std::nullopt);
}

base::expected<std::optional<OrchardShard>, OrchardStorage::Error>
OrchardStorage::LastShard(OrchardPool pool,
                          const mojom::AccountIdPtr& account_id,
                          uint8_t shard_height) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  auto shard_index = GetLatestShardIndex(pool, account_id);
  RETURN_IF_ERROR(shard_index);

  if (!shard_index.value()) {
    return base::ok(std::nullopt);
  }

  return GetShard(
      pool, account_id,
      OrchardShardAddress{shard_height, shard_index.value().value()});
}

base::expected<std::vector<OrchardShardAddress>, OrchardStorage::Error>
OrchardStorage::GetShardRoots(OrchardPool pool,
                              const mojom::AccountIdPtr& account_id,
                              uint8_t shard_level) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  std::vector<OrchardShardAddress> result;
  sql::Statement resolve_shards_statement(PrepareStatement(
      database_,
      absl::StrFormat("SELECT shard_index FROM %s WHERE account_id = ? "
                      "ORDER BY shard_index;",
                      tables.shard_tree.c_str())));
  resolve_shards_statement.BindString(0, account_id->unique_key);

  while (resolve_shards_statement.Step()) {
    auto shard_index = ReadUint32(resolve_shards_statement, 0);
    if (!shard_index) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong shard index format"});
    }
    result.push_back(OrchardShardAddress{shard_level, shard_index.value()});
  }

  if (!resolve_shards_statement.Succeeded()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(std::move(result));
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::AddCheckpoint(OrchardPool pool,
                              const mojom::AccountIdPtr& account_id,
                              uint32_t checkpoint_id,
                              const OrchardCheckpoint& checkpoint) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement extant_tree_state_statement(PrepareStatement(
      database_,
      absl::StrFormat(
          "SELECT position FROM %s WHERE checkpoint_id = ? AND account_id = ?;",
          tables.checkpoints.c_str())));
  extant_tree_state_statement.BindInt64(0, checkpoint_id);
  extant_tree_state_statement.BindString(1, account_id->unique_key);

  std::optional<CheckpointTreeState> extant_tree_state_position;
  if (extant_tree_state_statement.Step()) {
    auto state = ReadCheckpointTreeState(extant_tree_state_statement, 0);
    if (!state.has_value()) {
      return base::unexpected(Error{ErrorCode::kDbInitError, state.error()});
    }
    extant_tree_state_position = state.value();
  }

  if (!extant_tree_state_position) {
    sql::Statement insert_checkpoint_statement(PrepareStatement(
        database_,
        absl::StrFormat("INSERT INTO %s (account_id, checkpoint_id, position) "
                        "VALUES (?, ?, ?);",
                        tables.checkpoints.c_str())));
    insert_checkpoint_statement.BindString(0, account_id->unique_key);
    insert_checkpoint_statement.BindInt64(1, checkpoint_id);
    if (checkpoint.tree_state_position) {
      insert_checkpoint_statement.BindInt64(
          2, checkpoint.tree_state_position.value());
    } else {
      insert_checkpoint_statement.BindNull(2);
    }
    if (!insert_checkpoint_statement.Run()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }

    sql::Statement insert_marks_removed_statement(PrepareStatement(
        database_,
        absl::StrFormat("INSERT INTO %s "
                        "(account_id, checkpoint_id, mark_removed_position) "
                        "VALUES (?, ?, ?);",
                        tables.checkpoints_mark_removed.c_str())));
    for (const auto& mark : checkpoint.marks_removed) {
      insert_marks_removed_statement.Reset(true);
      insert_marks_removed_statement.BindString(0, account_id->unique_key);
      insert_marks_removed_statement.BindInt64(1, checkpoint_id);
      insert_marks_removed_statement.BindInt64(2, mark);

      if (!insert_marks_removed_statement.Run()) {
        return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                      database_.GetErrorMessage()});
      }
    }
  } else {
    if (extant_tree_state_position.value() != checkpoint.tree_state_position) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Tree state position differs"});
    }
    auto marks_removed_result =
        GetMarksRemoved(pool, account_id, checkpoint_id);
    RETURN_IF_ERROR(marks_removed_result);

    if (marks_removed_result.value() != checkpoint.marks_removed) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Marks removed differs"});
    }
  }

  return base::ok(Result::kSuccess);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::UpdateCheckpoint(OrchardPool pool,
                                 const mojom::AccountIdPtr& account_id,
                                 uint32_t checkpoint_id,
                                 const OrchardCheckpoint& checkpoint) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  ASSIGN_OR_RETURN(auto get_checkpoint_result,
                   GetCheckpoint(pool, account_id, checkpoint_id));
  if (!get_checkpoint_result.has_value()) {
    return base::ok(Result::kNone);
  }

  ASSIGN_OR_RETURN(auto remove_result,
                   RemoveCheckpoint(pool, account_id, checkpoint_id));
  if (remove_result != Result::kSuccess) {
    return base::ok(Result::kNone);
  }

  return AddCheckpoint(pool, account_id, checkpoint_id, checkpoint);
}

base::expected<size_t, OrchardStorage::Error> OrchardStorage::CheckpointCount(
    OrchardPool pool,
    const mojom::AccountIdPtr& account_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement resolve_checkpoints_count(PrepareStatement(
      database_,
      absl::StrFormat("SELECT COUNT(*) FROM %s WHERE account_id = ?;",
                      tables.checkpoints.c_str())));
  resolve_checkpoints_count.BindString(0, account_id->unique_key);
  if (!resolve_checkpoints_count.Step()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  auto value = ReadUint32(resolve_checkpoints_count, 0);
  if (!value) {
    return base::unexpected(
        Error{ErrorCode::kConsistencyError, "Wrong checkpoint count"});
  }

  return *value;
}

base::expected<std::optional<uint32_t>, OrchardStorage::Error>
OrchardStorage::MinCheckpointId(OrchardPool pool,
                                const mojom::AccountIdPtr& account_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement resolve_min_checkpoint_id(PrepareStatement(
      database_,
      absl::StrFormat("SELECT MIN(checkpoint_id) FROM %s WHERE account_id = ?;",
                      tables.checkpoints.c_str())));
  resolve_min_checkpoint_id.BindString(0, account_id->unique_key);

  if (!resolve_min_checkpoint_id.Step()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  if (resolve_min_checkpoint_id.GetColumnType(0) == sql::ColumnType::kNull) {
    return base::ok(std::nullopt);
  }

  auto checkpoint_id = ReadUint32(resolve_min_checkpoint_id, 0);
  if (!checkpoint_id) {
    return base::unexpected(
        Error{ErrorCode::kConsistencyError, "Wrong checkpoint id format"});
  }
  return *checkpoint_id;
}

base::expected<std::optional<uint32_t>, OrchardStorage::Error>
OrchardStorage::MaxCheckpointId(OrchardPool pool,
                                const mojom::AccountIdPtr& account_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement resolve_max_checkpoint_id(PrepareStatement(
      database_,
      absl::StrFormat("SELECT MAX(checkpoint_id) FROM %s WHERE account_id = ?;",
                      tables.checkpoints.c_str())));
  resolve_max_checkpoint_id.BindString(0, account_id->unique_key);

  if (!resolve_max_checkpoint_id.Step()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  if (resolve_max_checkpoint_id.GetColumnType(0) == sql::ColumnType::kNull) {
    return base::ok(std::nullopt);
  }

  auto checkpoint_id = ReadUint32(resolve_max_checkpoint_id, 0);
  if (!checkpoint_id) {
    return base::unexpected(
        Error{ErrorCode::kConsistencyError, "Wrong checkpoint id format"});
  }
  return *checkpoint_id;
}

base::expected<std::optional<uint32_t>, OrchardStorage::Error>
OrchardStorage::GetCheckpointAtDepth(OrchardPool pool,
                                     const mojom::AccountIdPtr& account_id,
                                     uint32_t depth) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement get_checkpoint_at_depth_statement(PrepareStatement(
      database_,
      absl::StrFormat("SELECT checkpoint_id FROM %s WHERE account_id = ? "
                      "ORDER BY checkpoint_id DESC LIMIT 1 OFFSET ?;",
                      tables.checkpoints.c_str())));
  get_checkpoint_at_depth_statement.BindString(0, account_id->unique_key);
  get_checkpoint_at_depth_statement.BindInt64(1, depth);

  if (get_checkpoint_at_depth_statement.Step()) {
    auto value = ReadUint32(get_checkpoint_at_depth_statement, 0);
    if (!value) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong checkpoint id format"});
    }
    return *value;
  }

  if (!get_checkpoint_at_depth_statement.Succeeded()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(std::nullopt);
}

base::expected<std::vector<uint32_t>, OrchardStorage::Error>
OrchardStorage::GetMarksRemoved(OrchardPool pool,
                                const mojom::AccountIdPtr& account_id,
                                uint32_t checkpoint_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement get_marks_removed_statement(PrepareStatement(
      database_, absl::StrFormat("SELECT mark_removed_position FROM %s "
                                 "WHERE checkpoint_id = ? AND account_id = ?;",
                                 tables.checkpoints_mark_removed.c_str())));
  get_marks_removed_statement.BindInt64(0, checkpoint_id);
  get_marks_removed_statement.BindString(1, account_id->unique_key);

  std::vector<uint32_t> result;
  while (get_marks_removed_statement.Step()) {
    auto position = ReadUint32(get_marks_removed_statement, 0);
    if (!position) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong position format"});
    }
    result.push_back(*position);
  }

  if (!get_marks_removed_statement.Succeeded()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(std::move(result));
}

base::expected<std::optional<OrchardCheckpointBundle>, OrchardStorage::Error>
OrchardStorage::GetCheckpoint(OrchardPool pool,
                              const mojom::AccountIdPtr& account_id,
                              uint32_t checkpoint_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement get_checkpoint_statement(PrepareStatement(
      database_, absl::StrFormat("SELECT position FROM %s "
                                 "WHERE checkpoint_id = ? AND account_id = ?;",
                                 tables.checkpoints.c_str())));
  get_checkpoint_statement.BindInt64(0, checkpoint_id);
  get_checkpoint_statement.BindString(1, account_id->unique_key);
  if (!get_checkpoint_statement.Step()) {
    if (!get_checkpoint_statement.Succeeded()) {
      return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                    database_.GetErrorMessage()});
    }
    return base::ok(std::nullopt);
  }
  auto checkpoint_position =
      ReadCheckpointTreeState(get_checkpoint_statement, 0);
  if (!checkpoint_position.has_value()) {
    return base::unexpected(
        Error{ErrorCode::kConsistencyError, "Wrong position format"});
  }

  sql::Statement marks_removed_statement(PrepareStatement(
      database_, absl::StrFormat("SELECT mark_removed_position FROM %s "
                                 "WHERE checkpoint_id = ? AND account_id = ?;",
                                 tables.checkpoints_mark_removed.c_str())));
  marks_removed_statement.BindInt64(0, checkpoint_id);
  marks_removed_statement.BindString(1, account_id->unique_key);

  std::vector<uint32_t> positions;
  while (marks_removed_statement.Step()) {
    auto position = ReadUint32(marks_removed_statement, 0);
    if (position) {
      positions.push_back(*position);
    } else {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong position format"});
    }
  }

  if (!marks_removed_statement.Succeeded()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return OrchardCheckpointBundle{
      checkpoint_id,
      OrchardCheckpoint{*checkpoint_position, std::move(positions)}};
}

base::expected<std::vector<OrchardCheckpointBundle>, OrchardStorage::Error>
OrchardStorage::GetCheckpoints(OrchardPool pool,
                               const mojom::AccountIdPtr& account_id,
                               size_t limit) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement get_checkpoints_statement(PrepareStatement(
      database_,
      absl::StrFormat(
          "SELECT checkpoint_id, position FROM %s WHERE account_id = ? "
          "ORDER BY position LIMIT ?;",
          tables.checkpoints.c_str())));
  get_checkpoints_statement.BindString(0, account_id->unique_key);
  get_checkpoints_statement.BindInt64(1, limit);

  std::vector<OrchardCheckpointBundle> checkpoints;
  while (get_checkpoints_statement.Step()) {
    auto checkpoint_id = ReadUint32(get_checkpoints_statement, 0);
    if (!checkpoint_id) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong checkpoint id format"});
    }

    auto checkpoint_position =
        ReadCheckpointTreeState(get_checkpoints_statement, 1);
    if (!checkpoint_position.has_value()) {
      return base::unexpected(Error{ErrorCode::kConsistencyError,
                                    "Wrong checkpoint position format"});
    }
    auto found_marks_removed =
        GetMarksRemoved(pool, account_id, *checkpoint_id);
    RETURN_IF_ERROR(found_marks_removed);
    checkpoints.push_back(OrchardCheckpointBundle{
        *checkpoint_id,
        OrchardCheckpoint(checkpoint_position.value(),
                          std::move(found_marks_removed.value()))});
  }

  if (!get_checkpoints_statement.Succeeded()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(std::move(checkpoints));
}

base::expected<std::optional<uint32_t>, OrchardStorage::Error>
OrchardStorage::GetMaxCheckpointedHeight(OrchardPool pool,
                                         const mojom::AccountIdPtr& account_id,
                                         uint32_t chain_tip_height,
                                         uint32_t min_confirmations) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    return base::unexpected(
        Error{ErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  uint32_t max_checkpointed_height = chain_tip_height - min_confirmations - 1;
  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement get_max_checkpointed_height(PrepareStatement(
      database_, absl::StrFormat("SELECT checkpoint_id FROM %s "
                                 "WHERE checkpoint_id <= ? AND account_id = ? "
                                 "ORDER BY checkpoint_id DESC LIMIT 1;",
                                 tables.checkpoints.c_str())));
  get_max_checkpointed_height.BindInt64(0, max_checkpointed_height);
  get_max_checkpointed_height.BindString(1, account_id->unique_key);

  if (get_max_checkpointed_height.Step()) {
    auto value = ReadUint32(get_max_checkpointed_height, 0);
    if (!value) {
      return base::unexpected(
          Error{ErrorCode::kConsistencyError, "Wrong checkpoint height"});
    }
    return base::ok(*value);
  }

  if (!get_max_checkpointed_height.Succeeded()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(std::nullopt);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::RemoveCheckpoint(OrchardPool pool,
                                 const mojom::AccountIdPtr& account_id,
                                 uint32_t checkpoint_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  ASSIGN_OR_RETURN(auto existing_checkpoint,
                   GetCheckpoint(pool, account_id, checkpoint_id));

  if (!existing_checkpoint.has_value()) {
    return base::ok(Result::kNone);
  }

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement remove_checkpoint_by_id(PrepareStatement(
      database_,
      absl::StrFormat(
          "DELETE FROM %s WHERE checkpoint_id = ? AND account_id = ?;",
          tables.checkpoints.c_str())));
  remove_checkpoint_by_id.BindInt64(0, checkpoint_id);
  remove_checkpoint_by_id.BindString(1, account_id->unique_key);

  if (!remove_checkpoint_by_id.Run()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(Result::kSuccess);
}

base::expected<OrchardStorage::Result, OrchardStorage::Error>
OrchardStorage::TruncateCheckpoints(OrchardPool pool,
                                    const mojom::AccountIdPtr& account_id,
                                    uint32_t checkpoint_id) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  const PoolTableNames tables = TablesForPool(pool);
  sql::Statement truncate_checkpoints(PrepareStatement(
      database_,
      absl::StrFormat(
          "DELETE FROM %s WHERE checkpoint_id >= ? AND account_id = ?;",
          tables.checkpoints.c_str())));
  truncate_checkpoints.BindInt64(0, checkpoint_id);
  truncate_checkpoints.BindString(1, account_id->unique_key);

  if (!truncate_checkpoints.Run()) {
    return base::unexpected(Error{ErrorCode::kFailedToExecuteStatement,
                                  database_.GetErrorMessage()});
  }

  return base::ok(Result::kSuccess);
}

}  // namespace brave_wallet
