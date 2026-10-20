// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/inbox_store.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/files/file_util.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/strcat.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace brave_transfer {

namespace {

constexpr int kCurrentVersion = 1;
constexpr int kCompatibleVersion = 1;

constexpr char kCreateTable[] =
    "CREATE TABLE IF NOT EXISTS items("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "received_time INTEGER NOT NULL,"
    "device_guid TEXT NOT NULL,"
    "device_name TEXT NOT NULL,"
    "kind INTEGER NOT NULL,"
    "text TEXT NOT NULL,"
    "file_path TEXT NOT NULL,"
    "file_name TEXT NOT NULL,"
    "mime_type TEXT NOT NULL,"
    "size INTEGER NOT NULL,"
    "sha256 TEXT NOT NULL,"
    "saved_to_fallback INTEGER NOT NULL)";

constexpr char kSelectColumns[] =
    "SELECT id, received_time, device_guid, device_name, kind, text, "
    "file_path, file_name, mime_type, size, sha256, saved_to_fallback "
    "FROM items";

}  // namespace

InboxEntry::InboxEntry() = default;
InboxEntry::InboxEntry(const InboxEntry&) = default;
InboxEntry& InboxEntry::operator=(const InboxEntry&) = default;
InboxEntry::InboxEntry(InboxEntry&&) = default;
InboxEntry& InboxEntry::operator=(InboxEntry&&) = default;
InboxEntry::~InboxEntry() = default;

InboxPage::InboxPage() = default;
InboxPage::InboxPage(InboxPage&&) = default;
InboxPage& InboxPage::operator=(InboxPage&&) = default;
InboxPage::~InboxPage() = default;

InboxStore::InboxStore(const base::FilePath& db_path)
    : db_path_(db_path),
      db_(sql::DatabaseOptions().set_page_size(4096).set_cache_size(64),
          sql::Database::Tag("BraveTransferInbox")) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

InboxStore::~InboxStore() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool InboxStore::LazyInit() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (init_result_.has_value()) {
    return *init_result_;
  }
  init_result_ = false;

  if (db_path_.empty()) {
    CHECK_IS_TEST();
    if (!db_.OpenInMemory()) {
      return false;
    }
  } else {
    if (!base::CreateDirectory(db_path_.DirName()) || !db_.Open(db_path_)) {
      return false;
    }
  }

  if (sql::MetaTable::RazeIfIncompatible(&db_, kCompatibleVersion,
                                         kCurrentVersion) ==
      sql::RazeIfIncompatibleResult::kFailed) {
    return false;
  }

  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    return false;
  }
  sql::MetaTable meta_table;
  if (!meta_table.Init(&db_, kCurrentVersion, kCompatibleVersion) ||
      !db_.Execute(kCreateTable) || !transaction.Commit()) {
    return false;
  }
  init_result_ = true;
  return true;
}

std::optional<int64_t> InboxStore::Add(InboxEntry entry) {
  if (!LazyInit()) {
    return std::nullopt;
  }
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE,
      "INSERT INTO items(received_time, device_guid, device_name, kind, text, "
      "file_path, file_name, mime_type, size, sha256, saved_to_fallback) "
      "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
  statement.BindTime(0, entry.received_time);
  statement.BindString(1, entry.device_guid);
  statement.BindString(2, entry.device_name);
  statement.BindInt(3, static_cast<int>(entry.kind));
  statement.BindString(4, entry.text);
  statement.BindString(5, entry.file_path.AsUTF8Unsafe());
  statement.BindString(6, entry.file_name);
  statement.BindString(7, entry.mime_type);
  statement.BindInt64(8, base::saturated_cast<int64_t>(entry.size));
  statement.BindString(9, entry.sha256);
  statement.BindBool(10, entry.saved_to_fallback);
  if (!statement.Run()) {
    return std::nullopt;
  }
  return db_.GetLastInsertRowId();
}

InboxEntry InboxStore::ReadEntry(sql::Statement& statement) const {
  InboxEntry entry;
  entry.id = statement.ColumnInt64(0);
  entry.received_time = statement.ColumnTime(1);
  entry.device_guid = statement.ColumnString(2);
  entry.device_name = statement.ColumnString(3);
  const auto kind = static_cast<mojom::ItemKind>(statement.ColumnInt(4));
  entry.kind = mojom::IsKnownEnumValue(kind) ? kind : mojom::ItemKind::kFile;
  entry.text = statement.ColumnString(5);
  entry.file_path = base::FilePath::FromUTF8Unsafe(statement.ColumnString(6));
  entry.file_name = statement.ColumnString(7);
  entry.mime_type = statement.ColumnString(8);
  entry.size = base::saturated_cast<uint64_t>(statement.ColumnInt64(9));
  entry.sha256 = statement.ColumnString(10);
  entry.saved_to_fallback = statement.ColumnBool(11);
  entry.file_missing =
      !entry.file_path.empty() && !base::PathExists(entry.file_path);
  return entry;
}

InboxPage InboxStore::Get(uint32_t offset, uint32_t count) {
  InboxPage page;
  if (!LazyInit() || count == 0) {
    return page;
  }
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE, base::StrCat({kSelectColumns,
                                   " ORDER BY received_time DESC, id DESC "
                                   "LIMIT ? OFFSET ?"})));
  statement.BindInt64(0, int64_t{count} + 1);
  statement.BindInt64(1, offset);
  while (statement.Step()) {
    if (page.entries.size() == count) {
      page.has_more = true;
      break;
    }
    page.entries.push_back(ReadEntry(statement));
  }
  return page;
}

std::optional<InboxEntry> InboxStore::GetById(int64_t id) {
  if (!LazyInit()) {
    return std::nullopt;
  }
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE, base::StrCat({kSelectColumns, " WHERE id = ?"})));
  statement.BindInt64(0, id);
  if (!statement.Step()) {
    return std::nullopt;
  }
  return ReadEntry(statement);
}

bool InboxStore::Clear() {
  if (!LazyInit()) {
    return false;
  }
  return db_.Execute("DELETE FROM items");
}

}  // namespace brave_transfer
