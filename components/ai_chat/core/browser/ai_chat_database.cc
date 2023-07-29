/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_database.h"

#include <utility>

#include "base/strings/string_split.h"
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
      !CreateConversationEntryTextTable() || !CreateSearchQueriesTable()) {
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
    int index = 0;
    conversation->id = statement.ColumnInt64(index++);
    conversation->title = statement.ColumnString(index++);
    conversation->page_url = GURL(statement.ColumnString(index++));
    conversation->page_contents = statement.ColumnString(index++);
    conversation->date = DeserializeTime(statement.ColumnInt64(index++));
    conversation_list.emplace_back(std::move(conversation));
  }

  return conversation_list;
}

std::vector<mojom::ConversationEntryPtr> AIChatDatabase::GetConversationEntries(
    int64_t conversation_id) {
  sql::Statement statement(GetDB().GetUniqueStatement(
      "SELECT conversation_entry.id, conversation_entry.date, "
      "conversation_entry.character_type, conversation_entry.action_type, "
      "conversation_entry.selected_text, text.id, text.date, text.text, "
      "text.conversation_entry_id, "
      "GROUP_CONCAT(DISTINCT search_queries.query) AS search_queries"
      " FROM conversation_entry"
      " JOIN conversation_entry_text as text"
      " JOIN search_queries"
      " ON conversation_entry.id = text.conversation_entry_id"
      " WHERE conversation_entry.conversation_id=?"
      " GROUP BY conversation_entry.id, text.id"
      " ORDER BY conversation_entry.date ASC"));

  statement.BindInt64(0, conversation_id);

  std::vector<mojom::ConversationEntryPtr> history;

  int64_t last_conversation_entry_id = -1;

  while (statement.Step()) {
    mojom::ConversationEntryTextPtr entry_text =
        mojom::ConversationEntryText::New();
    entry_text->id = statement.ColumnInt64(5);
    entry_text->date = DeserializeTime(statement.ColumnInt64(6));
    entry_text->text = statement.ColumnString(7);

    int64_t current_conversation_id = statement.ColumnInt64(0);

    if (last_conversation_entry_id == current_conversation_id) {
      // Update the last entry if it is the same conversation entry
      history.back()->texts.emplace_back(std::move(entry_text));
    } else {
      mojom::ConversationEntryPtr entry = mojom::ConversationEntry::New();
      entry->id = statement.ColumnInt64(0);
      entry->date = DeserializeTime(statement.ColumnInt64(1));
      entry->character_type =
          static_cast<mojom::CharacterType>(statement.ColumnInt(2));
      entry->action_type =
          static_cast<mojom::ActionType>(statement.ColumnInt(3));
      entry->selected_text = statement.ColumnString(4);

      // Parse search queries
      std::string search_queries_all = statement.ColumnString(9);
      if (!search_queries_all.empty()) {
        entry->events = std::vector<mojom::ConversationEntryEventPtr>();

        std::vector<std::string> search_queries =
            base::SplitString(search_queries_all, ",", base::TRIM_WHITESPACE,
                              base::SPLIT_WANT_NONEMPTY);
        entry->events->emplace_back(
            mojom::ConversationEntryEvent::NewSearchQueriesEvent(
                mojom::SearchQueriesEvent::New(std::move(search_queries))));
      }

      // Add the text to the new entry
      entry->texts = std::vector<mojom::ConversationEntryTextPtr>();
      entry->texts.emplace_back(std::move(entry_text));

      last_conversation_entry_id = entry->id;

      // Add the new entry to the history
      history.emplace_back(std::move(entry));
    }
  }

  return history;
}

int64_t AIChatDatabase::AddConversation(mojom::ConversationPtr conversation) {
  sql::Statement statement(GetDB().GetUniqueStatement(
      "INSERT INTO conversation(id, title, "
      "page_url, page_contents) VALUES(NULL, ?, ?, ?)"));
  CHECK(statement.is_valid());

  statement.BindString(0, conversation->title);
  if (conversation->page_url.has_value()) {
    statement.BindString(1, conversation->page_url->spec());
  } else {
    statement.BindNull(1);
  }

  if (conversation->page_contents.has_value()) {
    statement.BindString(2, conversation->page_contents.value());
  } else {
    statement.BindNull(2);
  }

  if (!statement.Run()) {
    DVLOG(0) << "Failed to execute 'conversation' insert statement"
             << db_.GetErrorMessage();
    return INT64_C(-1);
  }

  return GetDB().GetLastInsertRowId();
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
      "INSERT INTO conversation_entry(id, date, "
      "character_type, action_type, selected_text, "
      "conversation_id) VALUES(NULL, ?, ?, ?, ?, ?)"));
  CHECK(insert_conversation_entry_statement.is_valid());

  // ConversationEntry's date should always match first text's date
  int index = 0;
  insert_conversation_entry_statement.BindTimeDelta(
      index++, SerializeTimeToDelta(entry->texts[0]->date));
  insert_conversation_entry_statement.BindInt(
      index++, static_cast<int>(entry->character_type));
  insert_conversation_entry_statement.BindInt(
      index++, static_cast<int>(entry->action_type));

  if (entry->selected_text.has_value()) {
    insert_conversation_entry_statement.BindString(
        index++, entry->selected_text.value());
  } else {
    insert_conversation_entry_statement.BindNull(index++);
  }

  insert_conversation_entry_statement.BindInt64(index++, conversation_id);

  if (!insert_conversation_entry_statement.Run()) {
    DVLOG(0) << "Failed to execute 'conversation_entry' insert statement: "
             << db_.GetErrorMessage();
    return INT64_C(-1);
  }

  int64_t conversation_entry_row_id = GetDB().GetLastInsertRowId();

  for (mojom::ConversationEntryTextPtr& text : entry->texts) {
    // Add texts
    AddConversationEntryText(conversation_entry_row_id, std::move(text));
  }

  if (entry->events.has_value()) {
    for (const mojom::ConversationEntryEventPtr& event : *entry->events) {
      if (event->is_search_queries_event()) {
        std::vector<std::string> queries =
            event->get_search_queries_event()->search_queries;
        for (const std::string& query : queries) {
          // Add search queries
          AddSearchQuery(conversation_id, conversation_entry_row_id, query);
        }
      }
    }
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
                                 ") VALUES(NULL, ?, ?, ?)"));
  CHECK(insert_text_statement.is_valid());

  insert_text_statement.BindTimeDelta(0,
                                      SerializeTimeToDelta(entry_text->date));
  insert_text_statement.BindString(1, entry_text->text);
  insert_text_statement.BindInt64(2, conversation_entry_id);

  if (!insert_text_statement.Run()) {
    DVLOG(0) << "Failed to execute 'conversation_entry_text' insert statement: "
             << db_.GetErrorMessage();
    return INT64_C(-1);
  }

  return GetDB().GetLastInsertRowId();
}

int64_t AIChatDatabase::AddSearchQuery(int64_t conversation_id,
                                       int64_t conversation_entry_id,
                                       const std::string& query) {
  sql::Statement insert_search_query_statement(GetDB().GetUniqueStatement(
      "INSERT INTO search_queries"
      "(id, query, conversation_id, conversation_entry_id)"
      "VALUES(NULL, ?, ?, ?)"));
  CHECK(insert_search_query_statement.is_valid());

  int index = 0;
  insert_search_query_statement.BindString(index++, query);
  insert_search_query_statement.BindInt64(index++, conversation_id);
  insert_search_query_statement.BindInt64(index++, conversation_entry_id);

  if (!insert_search_query_statement.Run()) {
    DVLOG(0) << "Failed to execute 'search_queries' insert statement: "
             << db_.GetErrorMessage();
    return INT64_C(-1);
  }

  return GetDB().GetLastInsertRowId();
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
      "page_url TEXT,"
      "page_contents TEXT)");
}

bool AIChatDatabase::CreateConversationEntryTable() {
  return GetDB().Execute(
      "CREATE TABLE IF NOT EXISTS conversation_entry("
      "id INTEGER PRIMARY KEY,"
      "date INTEGER NOT NULL,"
      "character_type INTEGER NOT NULL,"
      "action_type INTEGER NOT NULL,"
      "selected_text TEXT,"
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

bool AIChatDatabase::CreateSearchQueriesTable() {
  return GetDB().Execute(
      "CREATE TABLE IF NOT EXISTS search_queries("
      "id INTEGER PRIMARY KEY,"
      "query TEXT NOT NULL,"
      "conversation_id INTEGER NOT NULL,"
      "conversation_entry_id INTEGER NOT NULL)");
}
}  // namespace ai_chat
