#include "brave/components/brave_wallet/browser/zcash/orchard_storage.h"

#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_wallet {

namespace {
constexpr char kDatabaseName[] = "orchard.db";

constexpr char kNotesTable[] = "notes";
constexpr char kSpentNotesTable[] = "spent_notes";
constexpr char kAccountMeta[] = "account_meta";

const int kEmptyDbVersionNumber = 1;
const int kCurrentVersionNumber = 2;
}  // namespace

OrchardStorage::OrchardStorage(base::FilePath path_to_database_dir)
    : db_file_path_(path_to_database_dir.Append(kDatabaseName)),
      database_(sql::DatabaseOptions{.page_size = 4096, .cache_size = 128}) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

OrchardStorage::~OrchardStorage() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool OrchardStorage::EnsureDbInit() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (database_.is_open()) {
    return true;
  }
  return CreateOrUpdateDatabase();
}

void OrchardStorage::ResetDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  database_.Close();
  sql::Database::Delete(db_file_path_);
}

bool OrchardStorage::CreateOrUpdateDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const base::FilePath dir = db_file_path_.DirName();
  if (!base::DirectoryExists(dir) && !base::CreateDirectory(dir)) {
    NOTREACHED();
    return false;
  }

  if (!database_.Open(db_file_path_)) {
    NOTREACHED();
    return false;
  }

  sql::MetaTable meta_table;
  if (!meta_table.Init(&database_, kEmptyDbVersionNumber,
                       kEmptyDbVersionNumber)) {
    NOTREACHED();
    database_.Close();
    return false;
  }

  if (meta_table.GetVersionNumber() == kEmptyDbVersionNumber) {
    if (!CreateSchema() ||
        !meta_table.SetVersionNumber(kCurrentVersionNumber)) {
      NOTREACHED();
      database_.Close();
      return false;
    }
  } else if (meta_table.GetVersionNumber() < kCurrentVersionNumber) {
    if (!UpdateSchema() ||
        !meta_table.SetVersionNumber(kCurrentVersionNumber)) {
      NOTREACHED();
      database_.Close();
      return false;
    }
  }

  return true;
}

bool OrchardStorage::CreateSchema() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  sql::Transaction transaction(&database_);
  return transaction.Begin() &&
         database_.Execute(
             base::StringPrintf("CREATE TABLE %s ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                "account_id TEXT NOT NULL,"
                                "amount TEXT NOT NULL,"
                                "block_id TEXT NOT NULL,"
                                "nullifier BLOB NOT NULL UNIQUE);",
                                kNotesTable)
                 .c_str()) &&
         database_.Execute(
             base::StringPrintf("CREATE TABLE %s ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                "account_id TEXT NOT NULL,"
                                "spent_block_id TEXT NOT NULL,"
                                "nullifier BLOB NOT NULL UNIQUE);",
                                kSpentNotesTable)
                 .c_str()) &&
         database_.Execute(
             base::StringPrintf("CREATE TABLE %s ("
                                "account_id TEXT NOT NULL PRIMARY KEY,"
                                "account_birthday TEXT NOT NULL,"
                                "latest_scanned_block TEXT NOT NULL,"
                                "latest_scanned_block_hash TEXT NOT NULL);",
                                kAccountMeta)
                 .c_str()) &&
         transaction.Commit();
}

bool OrchardStorage::UpdateSchema() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return true;
}

std::optional<OrchardStorage::OrchardStorageError>
OrchardStorage::RegisterAccount(
    mojom::AccountIdPtr account_id,
    uint64_t account_birthday_block,
    const std::string& account_birthday_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED();
    return OrchardStorageError{kDbInitError, "Failed to init database "};
  }

  sql::Transaction transaction(&database_);
  if (!transaction.Begin()) {
    NOTREACHED();
    return OrchardStorageError{kDbInitError, "Failed to init database "};
  }

  sql::Statement register_account_statement(database_.GetUniqueStatement(
      base::StringPrintf("INSERT OR REPLACE INTO %s "
                         "(account_id, account_birthday, latest_scanned_block, "
                         "latest_scanned_block_hash) "
                         "VALUES (?, ?, ?, ?)",
                         kAccountMeta)
          .c_str()));

  register_account_statement.BindString(0, account_id->unique_key);
  register_account_statement.BindString(
      1, base::NumberToString(account_birthday_block));
  register_account_statement.BindString(
      2, base::NumberToString(account_birthday_block));
  register_account_statement.BindString(3, account_birthday_block_hash);

  if (!register_account_statement.Run()) {
    transaction.Rollback();
    NOTREACHED();
    return OrchardStorageError{kFailedToExecuteStatement,
                               database_.GetErrorMessage()};
  }

  if (!transaction.Commit()) {
    return OrchardStorageError{kFailedToExecuteStatement,
                               database_.GetErrorMessage()};
  }

  return std::nullopt;
}

base::expected<AccountMeta, OrchardStorage::OrchardStorageError>
OrchardStorage::GetAccountMeta(mojom::AccountIdPtr account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED();
    return base::unexpected(
        OrchardStorageError{kDbInitError, "Failed to init database "});
  }

  sql::Statement resolve_account_statement(database_.GetUniqueStatement(
      base::StringPrintf(
          "SELECT latest_scanned_block, latest_scanned_block_hash FROM %s "
          "WHERE account_id = ?;",
          kAccountMeta)
          .c_str()));

  resolve_account_statement.BindString(0, account_id->unique_key);

  if (!resolve_account_statement.Step()) {
    return base::unexpected(
        OrchardStorageError{kAccountNotFound, "Account not found"});
  }

  AccountMeta account_meta;
  uint64_t latest_scanned_block;
  if (!base::StringToUint64(resolve_account_statement.ColumnString(0),
                            &latest_scanned_block)) {
    NOTREACHED();
    return base::unexpected(
        OrchardStorageError{kInternalError, "Internal error"});
  }
  account_meta.latest_scanned_block_id = latest_scanned_block;
  account_meta.latest_scanned_block_hash =
      resolve_account_statement.ColumnString(1);
  return account_meta;
}

std::optional<OrchardStorage::OrchardStorageError>
OrchardStorage::HandleChainReorg(mojom::AccountIdPtr account_id,
                                 uint64_t reorg_block_id,
                                 const std::string& reorg_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    NOTREACHED();
    return OrchardStorageError{OrchardStorageErrorCode::kDbInitError,
                               database_.GetErrorMessage()};
  }

  if (!database_.BeginTransaction()) {
    NOTREACHED();
    return OrchardStorageError{OrchardStorageErrorCode::kInternalError,
                               database_.GetErrorMessage()};
  }

  sql::Statement remove_from_spent_notes(database_.GetUniqueStatement(
      base::StringPrintf("DELETE FROM %s "
                         "WHERE block_id > ? AND account_id = ?;",
                         kSpentNotesTable)
          .c_str()));

  remove_from_spent_notes.BindString(0, base::NumberToString(reorg_block_id));
  remove_from_spent_notes.BindString(1, account_id->unique_key);

  sql::Statement remove_from_notes(database_.GetUniqueStatement(
      base::StringPrintf("DELETE FROM %s "
                         "WHERE block_id > ? AND account_id = ?;",
                         kNotesTable)
          .c_str()));

  remove_from_notes.BindString(0, base::NumberToString(reorg_block_id));
  remove_from_notes.BindString(1, account_id->unique_key);

  sql::Statement update_account_meta(database_.GetUniqueStatement(
      base::StringPrintf(
          "UPDATE %s "
          "SET latest_scanned_block = ?, latest_scanned_block_hash = ?"
          "WHERE account_id = ?;",
          kAccountMeta)
          .c_str()));

  update_account_meta.BindString(0, base::NumberToString(reorg_block_id));
  update_account_meta.BindString(1, reorg_block_hash);
  update_account_meta.BindString(2, account_id->unique_key);

  if (!remove_from_notes.Run() || !remove_from_spent_notes.Run() ||
      !update_account_meta.Run()) {
    NOTREACHED();
    database_.RollbackTransaction();
    return OrchardStorageError{
        OrchardStorageErrorCode::kFailedToExecuteStatement,
        database_.GetErrorMessage()};
  }

  if (!database_.CommitTransaction()) {
    return OrchardStorageError{
        OrchardStorageErrorCode::kFailedToExecuteStatement,
        database_.GetErrorMessage()};
  }

  return std::nullopt;
}

base::expected<std::vector<OrchardNote>, OrchardStorage::OrchardStorageError>
OrchardStorage::GetSpendableNotes(mojom::AccountIdPtr account_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!EnsureDbInit()) {
    NOTREACHED();
    return base::unexpected(OrchardStorageError{
        OrchardStorageErrorCode::kDbInitError, database_.GetErrorMessage()});
  }

  sql::Statement resolve_unspent_notes(database_.GetUniqueStatement(
      base::StringPrintf(
          "SELECT "
          "notes.block_id, notes.amount,"
          "notes.nullifier FROM %s LEFT OUTER JOIN spent_notes "
          "ON notes.nullifier = spent_notes.nullifier "
          "WHERE spent_notes.nullifier IS NULL AND notes.account_id = ?;",
          kNotesTable)
          .c_str()));

  resolve_unspent_notes.BindString(0, account_id->unique_key);

  std::vector<OrchardNote> result;
  while (resolve_unspent_notes.Step()) {
    OrchardNote note;
    if (!base::StringToUint64(resolve_unspent_notes.ColumnString(0),
                              &note.block_id)) {
      return base::unexpected(OrchardStorageError{
          OrchardStorageErrorCode::kFailedToExecuteStatement,
          database_.GetErrorMessage()});
    }
    if (!base::StringToUint64(resolve_unspent_notes.ColumnString(1),
                              &note.amount)) {
      return base::unexpected(OrchardStorageError{
          OrchardStorageErrorCode::kFailedToExecuteStatement,
          database_.GetErrorMessage()});
    }
    auto nullifier = resolve_unspent_notes.ColumnBlob(2);
    std::copy(nullifier.begin(), nullifier.end(), note.nullifier.begin());
    result.push_back(std::move(note));
  }
  return result;
}

std::optional<OrchardStorage::OrchardStorageError> OrchardStorage::UpdateNotes(
    mojom::AccountIdPtr account_id,
    const std::vector<OrchardNote>& found_notes,
    const std::vector<OrchardNullifier>& spent_notes,
    const uint64_t latest_scanned_block,
    const std::string& latest_scanned_block_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(account_id);

  if (!EnsureDbInit()) {
    NOTREACHED();
    return OrchardStorageError{OrchardStorageErrorCode::kDbInitError,
                               database_.GetErrorMessage()};
  }

  sql::Transaction transaction(&database_);
  if (!transaction.Begin()) {
    NOTREACHED();
    return OrchardStorageError{OrchardStorageErrorCode::kDbInitError,
                               database_.GetErrorMessage()};
  }

  // Insert found notes to the notes table
  sql::Statement statement_populate_notes(database_.GetUniqueStatement(
      base::StringPrintf("INSERT OR REPLACE INTO %s "
                         "(account_id, amount, block_id, nullifier) "
                         "VALUES (?, ?, ?, ?);",
                         kNotesTable)
          .c_str()));

  for (const auto& note : found_notes) {
    statement_populate_notes.Reset(true);
    statement_populate_notes.BindString(0, account_id->unique_key);
    statement_populate_notes.BindString(1, base::NumberToString(note.amount));
    statement_populate_notes.BindString(2, base::NumberToString(note.block_id));
    statement_populate_notes.BindBlob(3, note.nullifier);
    if (!statement_populate_notes.Run()) {
      NOTREACHED();
      transaction.Rollback();
      return OrchardStorageError{
          OrchardStorageErrorCode::kFailedToExecuteStatement,
          database_.GetErrorMessage()};
    }
  }

  // Insert found spent nullifiers to spent notes table
  sql::Statement statement_populate_spent_notes(database_.GetUniqueStatement(
      base::StringPrintf("INSERT OR REPLACE INTO %s "
                         "(account_id, spent_block_id, nullifier) "
                         "VALUES (?, ?);",
                         kSpentNotesTable)
          .c_str()));

  for (const auto& note : spent_notes) {
    statement_populate_spent_notes.Reset(true);
    statement_populate_notes.BindString(0, account_id->address);
    statement_populate_spent_notes.BindString(
        1, base::NumberToString(note.block_id));
    statement_populate_spent_notes.BindBlob(2, note.nullifier);
    if (!statement_populate_spent_notes.Run()) {
      transaction.Rollback();
      NOTREACHED();
      return OrchardStorageError{
          OrchardStorageErrorCode::kFailedToExecuteStatement,
          database_.GetErrorMessage()};
    }
  }

  // Update account meta
  sql::Statement statement_update_account_meta(database_.GetUniqueStatement(
      base::StringPrintf(
          "UPDATE %s "
          "SET latest_scanned_block = ?, latest_scanned_block_hash = ?"
          "WHERE account_id = ?;",
          kAccountMeta)
          .c_str()));

  statement_update_account_meta.BindString(
      0, base::NumberToString(latest_scanned_block));
  statement_update_account_meta.BindString(1, latest_scanned_block_hash);
  statement_update_account_meta.BindString(2, account_id->unique_key);
  if (!statement_update_account_meta.Run()) {
    transaction.Rollback();
    DLOG(ERROR) << "Failed to execute statement";
    NOTREACHED();
    return OrchardStorageError{
        OrchardStorageErrorCode::kFailedToExecuteStatement,
        database_.GetErrorMessage()};
  }

  if (!transaction.Commit()) {
    return OrchardStorageError{
        OrchardStorageErrorCode::kFailedToExecuteStatement,
        database_.GetErrorMessage()};
  }

  return std::nullopt;
}

}  // namespace brave_wallet
