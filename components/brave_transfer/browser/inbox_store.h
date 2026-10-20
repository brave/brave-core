// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_INBOX_STORE_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_INBOX_STORE_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "brave/components/brave_transfer/common/brave_transfer.mojom.h"
#include "sql/database.h"

namespace sql {
class Statement;
}  // namespace sql

namespace brave_transfer {

// One received item.
struct InboxEntry {
  InboxEntry();
  InboxEntry(const InboxEntry&);
  InboxEntry& operator=(const InboxEntry&);
  InboxEntry(InboxEntry&&);
  InboxEntry& operator=(InboxEntry&&);
  ~InboxEntry();

  // Assigned by InboxStore::Add().
  int64_t id = 0;
  base::Time received_time;
  std::string device_guid;
  // The sender's name when the item was received.
  std::string device_name;
  mojom::ItemKind kind = mojom::ItemKind::kText;
  // Text or URL for text and link items.
  std::string text;
  // Saved file for image and file items.
  base::FilePath file_path;
  std::string file_name;
  std::string mime_type;
  uint64_t size = 0;
  // Lowercase hex SHA-256 of the content, or empty for inline items.
  std::string sha256;
  bool saved_to_fallback = false;
  // Not stored: set when reading if `file_path` no longer exists.
  bool file_missing = false;
};

struct InboxPage {
  InboxPage();
  InboxPage(InboxPage&&);
  InboxPage& operator=(InboxPage&&);
  ~InboxPage();

  std::vector<InboxEntry> entries;
  bool has_more = false;
};

// The inbox table (design plan §4.4) in a SQLite database. Blocking: use it
// on a background sequence, for example through base::SequenceBound. Entries
// are kept until Clear(); clearing never touches the saved files.
class InboxStore {
 public:
  static constexpr char kDirectoryName[] = "BraveTransfer";
  static constexpr char kFileName[] = "inbox.db";

  // `db_path` is opened on first use. An empty path uses an in-memory
  // database.
  explicit InboxStore(const base::FilePath& db_path);
  InboxStore(const InboxStore&) = delete;
  InboxStore& operator=(const InboxStore&) = delete;
  ~InboxStore();

  // Returns the new entry's id.
  std::optional<int64_t> Add(InboxEntry entry);
  // Newest first.
  InboxPage Get(uint32_t offset, uint32_t count);
  std::optional<InboxEntry> GetById(int64_t id);
  bool Clear();

 private:
  bool LazyInit();
  InboxEntry ReadEntry(sql::Statement& statement) const;

  const base::FilePath db_path_;
  sql::Database db_;
  std::optional<bool> init_result_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_INBOX_STORE_H_
