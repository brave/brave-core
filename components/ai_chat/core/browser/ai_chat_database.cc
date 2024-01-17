/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_database.h"

#include <utility>

#include "base/time/time.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {
// Do not use To/FromInternalValues in base::Time
// https://bugs.chromium.org/p/chromium/issues/detail?id=634507#c23

base::TimeDelta SerializeTimeToDelta(const base::Time& time) {
  return time.ToDeltaSinceWindowsEpoch();
}

base::Time DeserializeTime(const int64_t& serialized_time) {
  return base::Time() + base::Microseconds(serialized_time);
}

}  // namespace

namespace ai_chat {
AIChatDatabase::AIChatDatabase()
    : db_({.page_size = 4096, .cache_size = 1000}) {}

AIChatDatabase::~AIChatDatabase() = default;

bool AIChatDatabase::Init(const base::FilePath& db_file_path) {
  if (!GetDB().Open(db_file_path)) {
    return false;
  }

  if (!CreateConversationTable() || !CreateConversationEntryTable() ||
      !CreateConversationEntryTextTable()) {
    DVLOG(0) << "Failure to create tables\n";
    return false;
  }

  return true;
}

std::vector<mojom::ConversationPtr> AIChatDatabase::GetAllConversations() {
  sql::Statement statement(GetDB().GetUniqueStatement(
      "SELECT conversation.*, entries.date "
      "FROM conversation"
      " LEFT JOIN ("
      "SELECT conversation_id, date"
      " FROM conversation_entry"
      " GROUP BY conversation_id"
      " ORDER BY date DESC"
      " LIMIT 1"
      ") AS entries"
      " ON conversation.id = entries.conversation_id"));

  std::vector<mojom::ConversationPtr> conversation_list;

  while (statement.Step()) {
    mojom::ConversationPtr conversation = mojom::Conversation::New();
    conversation->id = statement.ColumnInt64(0);
    conversation->title = statement.ColumnString(1);
    conversation->page_url = GURL(statement.ColumnString(2));
    conversation->date = DeserializeTime(statement.ColumnInt64(3));
    conversation_list.emplace_back(std::move(conversation));
  }

  return conversation_list;
}

std::vector<mojom::ConversationEntryPtr> AIChatDatabase::GetConversationEntries(
    int64_t conversation_id) {
  sql::Statement statement(GetDB().GetUniqueStatement(
      "SELECT conversation_entry.*, entry_text.* "
      "FROM conversation_entry"
      " JOIN ("
      " SELECT *"
      " FROM conversation_entry_text"
      ") AS entry_text"
      " ON conversation_entry.id = entry_text.conversation_entry_id"
      " WHERE conversation_id=?"
      " ORDER BY conversation_entry.date ASC"));

  statement.BindInt64(0, conversation_id);

  std::vector<mojom::ConversationEntryPtr> history;

  while (statement.Step()) {
    mojom::ConversationEntryTextPtr entry_text =
        mojom::ConversationEntryText::New();
    entry_text->id = statement.ColumnInt64(4);
    entry_text->date = DeserializeTime(statement.ColumnInt64(5));
    entry_text->text = statement.ColumnString(6);

    int64_t conversation_entry_id = statement.ColumnInt64(7);

    auto iter = base::ranges::find_if(
        history,
        [&conversation_entry_id](const mojom::ConversationEntryPtr& entry) {
          return entry->id = conversation_entry_id;
        });

    // A ConversationEntry can include multiple generated texts
    if (iter != history.end()) {
      iter->get()->texts.emplace_back(std::move(entry_text));
      continue;
    }

    mojom::ConversationEntryPtr entry = mojom::ConversationEntry::New();
    entry->id = statement.ColumnInt64(0);
    entry->date = DeserializeTime(statement.ColumnInt64(1));
    entry->character_type =
        static_cast<mojom::CharacterType>(statement.ColumnInt(2));
    entry->texts.emplace_back(std::move(entry_text));

    history.emplace_back(std::move(entry));
  }

  return history;
}

int64_t AIChatDatabase::AddConversation(mojom::ConversationPtr conversation) {
  sql::Statement statement(
      GetDB().GetUniqueStatement("INSERT INTO conversation(id, title, "
                                 "page_url) VALUES(NULL, ?, ?) RETURNING id"));
  CHECK(statement.is_valid());

  statement.BindString(0, conversation->title);
  if (conversation->page_url.has_value()) {
    statement.BindString(1, conversation->page_url->spec());
  } else {
    statement.BindNull(1);
  }

  if (!statement.Step()) {
    DVLOG(0) << "Failed to execute 'conversation' insert statement"
             << db_.GetErrorMessage();
    return INT64_C(-1);
  }

  return statement.ColumnInt64(0);
}

int64_t AIChatDatabase::AddConversationEntry(
    int64_t conversation_id,
    mojom::ConversationEntryPtr entry) {
  sql::Transaction transaction(&GetDB());

  CHECK(GetDB().is_open());

  if (!transaction.Begin()) {
    DVLOG(0) << "Transaction cannot begin\n";
    return INT64_C(-1);
  }

  sql::Statement get_conversation_id_statement(
      GetDB().GetUniqueStatement("SELECT id FROM conversation"
                                 " WHERE id=?"));
  CHECK(get_conversation_id_statement.is_valid());

  get_conversation_id_statement.BindInt64(0, conversation_id);

  if (!get_conversation_id_statement.Step()) {
    DVLOG(0) << "ID not found in 'conversation' table";
    return INT64_C(-1);
  }

  sql::Statement insert_conversation_entry_statement(GetDB().GetUniqueStatement(
      "INSERT INTO conversation_entry(id, date, character_type, "
      "conversation_id) VALUES(1, ?, ?, ?)"));
  CHECK(insert_conversation_entry_statement.is_valid());

  // ConversationEntry's date should always match first text's date
  insert_conversation_entry_statement.BindTimeDelta(
      0, SerializeTimeToDelta(entry->texts[0]->date));
  insert_conversation_entry_statement.BindInt(
      1, static_cast<int>(entry->character_type));
  insert_conversation_entry_statement.BindInt64(
      2, get_conversation_id_statement.ColumnInt64(0));

  if (!insert_conversation_entry_statement.Step()) {
    DVLOG(0) << "Failed to execute 'conversation_entry' insert statement: "
             << db_.GetErrorMessage();
    return INT64_C(-1);
  }

  int64_t conversation_entry_row_id =
      insert_conversation_entry_statement.ColumnInt64(0);

  for (mojom::ConversationEntryTextPtr& text : entry->texts) {
    AddConversationEntryText(conversation_entry_row_id, std::move(text));
  }

  if (!transaction.Commit()) {
    DVLOG(0) << "Transaction commit failed with reason: "
             << db_.GetErrorMessage();
    return INT64_C(-1);
  }

  return conversation_entry_row_id;
}

int64_t AIChatDatabase::AddConversationEntryText(
    int64_t conversation_entry_id,
    mojom::ConversationEntryTextPtr entry_text) {
  sql::Statement get_conversation_entry_statement(
      GetDB().GetUniqueStatement("SELECT id FROM conversation_entry"
                                 " WHERE id=?"));
  CHECK(get_conversation_entry_statement.is_valid());
  get_conversation_entry_statement.BindInt64(0, conversation_entry_id);

  if (!get_conversation_entry_statement.Step()) {
    DVLOG(0) << "ID not found in 'conversation entry' table"
             << db_.GetErrorMessage();
    return INT64_C(-1);
  }

  sql::Statement insert_text_statement(
      GetDB().GetUniqueStatement("INSERT INTO "
                                 "conversation_entry_text("
                                 "id, date, text, conversation_entry_id"
                                 ") VALUES(NULL, ?, ?, ?) RETURNING id"));
  CHECK(insert_text_statement.is_valid());

  insert_text_statement.BindTimeDelta(0,
                                      SerializeTimeToDelta(entry_text->date));
  insert_text_statement.BindString(1, entry_text->text);
  insert_text_statement.BindInt64(2, conversation_entry_id);

  if (!insert_text_statement.Step()) {
    DVLOG(0) << "Failed to execute 'conversation_entry_text' insert statement: "
             << db_.GetErrorMessage();
    return INT64_C(-1);
  }

  return insert_text_statement.ColumnInt64(0);
}

bool AIChatDatabase::DeleteConversation(int64_t conversation_id) {
  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    DVLOG(0) << "Transaction cannot begin\n";
    return false;
  }

  sql::Statement delete_conversation_statement(
      GetDB().GetUniqueStatement("DELETE FROM conversation"
                                 "WHERE id=?"));
  delete_conversation_statement.BindInt64(0, conversation_id);

  if (!delete_conversation_statement.Run()) {
    return false;
  }

  sql::Statement select_conversation_entry_statement(
      GetDB().GetUniqueStatement("SELECT id FROM conversation_entry"
                                 "WHERE conversation_id=?"));

  // We remove all conversation text associated to |conversation_id|
  while (select_conversation_entry_statement.Step()) {
    sql::Statement delete_conversation_text_statement(
        GetDB().GetUniqueStatement("DELETE FROM conversation_entry_text"
                                   "WHERE conversation_entry_id=?"));
    delete_conversation_text_statement.BindInt64(
        0, select_conversation_entry_statement.ColumnInt64(0));

    if (!delete_conversation_text_statement.Run()) {
      return false;
    }
  }

  sql::Statement delete_conversation_entry_statement(
      GetDB().GetUniqueStatement("DELETE FROM conversation_entry"
                                 "WHERE conversation_entry_id=?"));
  delete_conversation_entry_statement.BindInt64(0, conversation_id);

  // At last we remove all conversation entries associated to |conversation_id|
  if (!delete_conversation_entry_statement.Run()) {
    return false;
  }

  transaction.Commit();
  return true;
}

bool AIChatDatabase::DropAllTables() {
  return GetDB().Execute("DROP TABLE conversation") &&
         GetDB().Execute("DROP TABLE conversation_entry") &&
         GetDB().Execute("DROP TABLE conversation_entry_text");
}

sql::Database& AIChatDatabase::GetDB() {
  return db_;
}

bool AIChatDatabase::CreateConversationTable() {
  return GetDB().Execute(
      "CREATE TABLE IF NOT EXISTS conversation("
      "id INTEGER PRIMARY KEY,"
      "title TEXT,"
      "page_url TEXT)");
}

bool AIChatDatabase::CreateConversationEntryTable() {
  return GetDB().Execute(
      "CREATE TABLE IF NOT EXISTS conversation_entry("
      "id INTEGER PRIMARY KEY,"
      "date INTEGER NOT NULL,"
      "character_type INTEGER NOT NULL,"
      "conversation_id INTEGER NOT NULL)");
}

bool AIChatDatabase::CreateConversationEntryTextTable() {
  return GetDB().Execute(
      "CREATE TABLE IF NOT EXISTS conversation_entry_text("
      "id INTEGER PRIMARY KEY,"
      "date INTEGER NOT NULL,"
      "text TEXT NOT NULL,"
      "conversation_entry_id INTEGER NOT NULL)");
}
}  // namespace ai_chat
