/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_database.h"

#include <map>
#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/strings/string_split.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace {

constexpr char kSearchQueriesSeparator[] = "|||";

std::optional<std::string> GetOptionalString(sql::Statement& statement,
                                             int index) {
  if (statement.GetColumnType(index) == sql::ColumnType::kNull) {
    return std::nullopt;
  }
  return std::make_optional(statement.ColumnString(index));
}

void BindOptionalString(sql::Statement& statement,
                        int index,
                        const std::optional<std::string>& value) {
  if (value.has_value() && !value.value().empty()) {
    statement.BindString(index, value.value());
  } else {
    statement.BindNull(index);
  }
}

bool MigrateFrom1To2(sql::Database* db) {
  // Add a new column to the associated_content table to store the content type.
  static constexpr char kAddPromptColumnQuery[] =
      "ALTER TABLE conversation_entry ADD COLUMN prompt BLOB";
  sql::Statement statement(db->GetUniqueStatement(kAddPromptColumnQuery));

  return statement.is_valid() && statement.Run();
}

}  // namespace

namespace ai_chat {

// These database versions should roll together unless we develop migrations.
// Lowest version we support migrations from - existing database will be deleted
// if lower.
constexpr int kLowestSupportedDatabaseVersion = 1;

// The oldest version of the schema such that a legacy Brave client using that
// version can still read/write the current database.
constexpr int kCompatibleDatabaseVersionNumber = 1;

// Current version of the database. Increase if breaking changes are made.
constexpr int kCurrentDatabaseVersion = 2;

AIChatDatabase::AIChatDatabase(const base::FilePath& db_file_path,
                               os_crypt_async::Encryptor encryptor)
    : db_file_path_(db_file_path),
      db_({.page_size = 4096, .cache_size = 1000}),
      encryptor_(std::move(encryptor)) {}

AIChatDatabase::~AIChatDatabase() = default;

bool AIChatDatabase::LazyInit(bool re_init) {
  if (!db_init_status_.has_value() || re_init) {
    db_init_status_ = InitInternal();
  }

  return *db_init_status_ == sql::InitStatus::INIT_OK;
}

sql::InitStatus AIChatDatabase::InitInternal() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!GetDB().is_open() && !GetDB().Open(db_file_path_)) {
    return sql::InitStatus::INIT_FAILURE;
  }

  if (sql::MetaTable::RazeIfIncompatible(
          &GetDB(), kLowestSupportedDatabaseVersion, kCurrentDatabaseVersion) ==
      sql::RazeIfIncompatibleResult::kFailed) {
    return sql::InitStatus::INIT_FAILURE;
  }

  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    return sql::InitStatus::INIT_FAILURE;
  }

  sql::MetaTable meta_table;
  if (!meta_table.Init(&GetDB(), kCurrentDatabaseVersion,
                       kCompatibleDatabaseVersionNumber)) {
    DVLOG(0) << "Failed to init meta table";
    return sql::InitStatus::INIT_FAILURE;
  }

  if (meta_table.GetCompatibleVersionNumber() > kCurrentDatabaseVersion) {
    LOG(ERROR) << "AIChat database version is too new.";
    return sql::InitStatus::INIT_TOO_NEW;
  }

  if (!CreateSchema()) {
    DVLOG(0) << "Failure to create tables";
    return sql::InitStatus::INIT_FAILURE;
  }

  if (meta_table.GetVersionNumber() < kCurrentDatabaseVersion) {
    bool migration_success = true;
    if (meta_table.GetVersionNumber() == 1) {
      migration_success = MigrateFrom1To2(&GetDB());
      migration_success = meta_table.SetCompatibleVersionNumber(
                              kCompatibleDatabaseVersionNumber) &&
                          meta_table.SetVersionNumber(kCurrentDatabaseVersion);
    }
    // Migration unsuccessful, raze the database and re-init
    if (!migration_success) {
      if (db_.Raze()) {
        return InitInternal();
      }
      return sql::InitStatus::INIT_FAILURE;
    }
  }

  if (!transaction.Commit()) {
    return sql::InitStatus::INIT_FAILURE;
  }

  return sql::InitStatus::INIT_OK;
}

std::vector<mojom::ConversationPtr> AIChatDatabase::GetAllConversations() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return {};
  }
  // All conversation metadata, associated content and most
  // and most recent entry date. 1 row for each associated content.
  static constexpr char kQuery[] =
      "SELECT conversation.uuid, conversation.title, conversation.model_key,"
      "  last_activity_date.date,"
      "  associated_content.uuid, associated_content.title,"
      "  associated_content.url, associated_content.content_type,"
      "  associated_content.content_used_percentage,"
      "  associated_content.is_content_refined"
      " FROM conversation"
      " LEFT JOIN associated_content"
      " ON conversation.uuid = associated_content.conversation_uuid"
      " LEFT JOIN ("
      "  SELECT conversation_entry.date AS date, "
      "  conversation_entry.conversation_uuid AS conversation_uuid "
      "  FROM conversation_entry"
      "  GROUP BY conversation_entry.conversation_uuid"
      "  ORDER BY conversation_entry.date desc) "
      " AS last_activity_date"
      " ON last_activity_date.conversation_uuid = conversation.uuid"
      " ORDER BY conversation.uuid ASC";
  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE, kQuery));
  CHECK(statement.is_valid());

  std::vector<mojom::ConversationPtr> conversation_list;
  // This/last row's conversation
  mojom::ConversationPtr conversation;

  while (statement.Step()) {
    DVLOG(1) << __func__ << " got a result";
    std::string uuid = statement.ColumnString(0);
    if (conversation) {
      if (conversation->uuid == uuid) {
        // TODO(petemill): Support multiple associated content
        continue;
      } else {
        conversation_list.emplace_back(std::move(conversation));
      }
    }
    auto index = 1;
    conversation = mojom::Conversation::New();
    conversation->uuid = uuid;
    conversation->title =
        DecryptOptionalColumnToString(statement, index++).value_or("");
    conversation->model_key = GetOptionalString(statement, index++);
    conversation->updated_time = statement.ColumnTime(index++);
    conversation->has_content = true;

    conversation->associated_content = mojom::SiteInfo::New();

    if (statement.GetColumnType(index) != sql::ColumnType::kNull) {
      DVLOG(1) << __func__ << " got associated content";

      conversation->associated_content->uuid = statement.ColumnString(index++);
      conversation->associated_content->title =
          DecryptOptionalColumnToString(statement, index++);
      auto url_raw = DecryptOptionalColumnToString(statement, index++);
      if (url_raw.has_value()) {
        conversation->associated_content->url = GURL(url_raw.value());
      }
      conversation->associated_content->content_type =
          static_cast<mojom::ContentType>(statement.ColumnInt(index++));
      conversation->associated_content->content_used_percentage =
          statement.ColumnInt(index++);
      conversation->associated_content->is_content_refined =
          statement.ColumnBool(index++);
      conversation->associated_content->is_content_association_possible = true;
    } else {
      conversation->associated_content->is_content_association_possible = false;
    }
  }

  // Final row's conversation
  if (conversation) {
    conversation_list.emplace_back(std::move(conversation));
  }

  return conversation_list;
}

mojom::ConversationArchivePtr AIChatDatabase::GetConversationData(
    std::string_view conversation_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return nullptr;
  }

  return mojom::ConversationArchive::New(
      GetConversationEntries(conversation_uuid),
      GetArchiveContentsForConversation(conversation_uuid));
}

std::vector<mojom::ConversationTurnPtr> AIChatDatabase::GetConversationEntries(
    std::string_view conversation_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  static constexpr char kEntriesQuery[] =
      "SELECT uuid, date, entry_text, prompt, character_type, "
      "editing_entry_uuid, "
      "action_type, selected_text"
      " FROM conversation_entry"
      " WHERE conversation_uuid=?"
      " ORDER BY date ASC";
  sql::Statement statement(
      GetDB().GetCachedStatement(SQL_FROM_HERE, kEntriesQuery));
  CHECK(statement.is_valid());

  statement.BindString(0, conversation_uuid);

  DVLOG(4) << __func__ << " for " << conversation_uuid;

  std::vector<mojom::ConversationTurnPtr> history;
  // Map of editing entry id to the edit entry
  std::map<std::string, std::vector<mojom::ConversationTurnPtr>> edits;

  while (statement.Step()) {
    // basic metadata
    std::string entry_uuid = statement.ColumnString(0);
    DVLOG(4) << "Found entry row for conversation " << conversation_uuid
             << " with id " << entry_uuid;
    int index = 1;
    auto date = statement.ColumnTime(index++);
    auto text = DecryptOptionalColumnToString(statement, index++).value_or("");
    auto prompt = DecryptOptionalColumnToString(statement, index++);
    auto character_type =
        static_cast<mojom::CharacterType>(statement.ColumnInt(index++));
    auto editing_entry_id = GetOptionalString(statement, index++);
    auto action_type =
        static_cast<mojom::ActionType>(statement.ColumnInt(index++));
    auto selected_text = DecryptOptionalColumnToString(statement, index++);

    auto entry = mojom::ConversationTurn::New(
        entry_uuid, character_type, action_type, text, prompt, selected_text,
        std::nullopt, date, std::nullopt, false);

    // events
    struct Event {
      int event_order;
      mojom::ConversationEntryEventPtr event;
    };
    std::vector<Event> events;

    // Completion events
    {
      sql::Statement event_statement(
          GetDB().GetCachedStatement(SQL_FROM_HERE,
                                     "SELECT event_order, text"
                                     " FROM conversation_entry_event_completion"
                                     " WHERE conversation_entry_uuid=?"
                                     " ORDER BY event_order ASC"));
      event_statement.BindString(0, entry_uuid);

      while (event_statement.Step()) {
        int event_order = event_statement.ColumnInt(0);
        std::string completion = DecryptColumnToString(event_statement, 1);
        events.emplace_back(Event{
            event_order, mojom::ConversationEntryEvent::NewCompletionEvent(
                             mojom::CompletionEvent::New(completion))});
      }
    }

    // Search Query events
    {
      sql::Statement event_statement(GetDB().GetUniqueStatement(
          "SELECT event_order, queries"
          " FROM conversation_entry_event_search_queries"
          " WHERE conversation_entry_uuid=?"
          " ORDER BY event_order ASC"));
      event_statement.BindString(0, entry_uuid);

      while (event_statement.Step()) {
        int event_order = event_statement.ColumnInt(0);
        auto queries_data = DecryptColumnToString(event_statement, 1);
        std::vector<std::string> queries =
            base::SplitString(queries_data, kSearchQueriesSeparator,
                              base::WhitespaceHandling::TRIM_WHITESPACE,
                              base::SplitResult::SPLIT_WANT_NONEMPTY);
        events.emplace_back(Event{
            event_order, mojom::ConversationEntryEvent::NewSearchQueriesEvent(
                             mojom::SearchQueriesEvent::New(queries))});
      }
    }

    // insert events in order
    if (!events.empty()) {
      base::ranges::sort(events, [](const Event& a, const Event& b) {
        return a.event_order < b.event_order;
      });
      entry->events = std::vector<mojom::ConversationEntryEventPtr>{};
      for (auto& event : events) {
        entry->events->emplace_back(std::move(event.event));
      }
    }

    // root entry or edited entry
    if (editing_entry_id.has_value()) {
      DVLOG(4) << "Collected edit entry for " << editing_entry_id.value()
               << " with id " << entry_uuid;
      edits[editing_entry_id.value()].emplace_back(std::move(entry));
    } else {
      DVLOG(4) << "Collected entry for " << entry_uuid;
      history.emplace_back(std::move(entry));
    }
  }

  // Reconstruct edits
  for (auto& entry : history) {
    CHECK(entry->uuid.has_value());
    auto id = entry->uuid.value();
    if (edits.count(id)) {
      entry->edits = std::vector<mojom::ConversationTurnPtr>{};
      for (auto& edit : edits[id]) {
        entry->edits->emplace_back(std::move(edit));
      }
    }
  }

  return history;
}

std::vector<mojom::ContentArchivePtr>
AIChatDatabase::GetArchiveContentsForConversation(
    std::string_view conversation_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  static constexpr char kQuery[] =
      "SELECT uuid, last_contents"
      " FROM associated_content"
      " WHERE conversation_uuid=?"
      " AND last_contents IS NOT NULL"
      " ORDER BY uuid ASC";
  sql::Statement statement(GetDB().GetCachedStatement(SQL_FROM_HERE, kQuery));
  CHECK(statement.is_valid());
  statement.BindString(0, conversation_uuid);
  std::vector<mojom::ContentArchivePtr> archive_contents;
  // We only support a single entry until ConversationHandler supports multiple
  // associated contents.
  if (statement.Step()) {
    auto content = mojom::ContentArchive::New(
        statement.ColumnString(0), DecryptColumnToString(statement, 1));
    archive_contents.emplace_back(std::move(content));
  }
  return archive_contents;
}

bool AIChatDatabase::AddConversation(mojom::ConversationPtr conversation,
                                     std::optional<std::string> contents,
                                     mojom::ConversationTurnPtr first_entry) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!conversation->uuid.empty());
  CHECK(first_entry);
  if (!LazyInit()) {
    return false;
  }

  sql::Transaction transaction(&GetDB());
  CHECK(GetDB().is_open());
  if (!transaction.Begin()) {
    DVLOG(0) << "Transaction cannot begin";
    return false;
  }

  static constexpr char kInsertConversationQuery[] =
      "INSERT INTO conversation(uuid, title, model_key) "
      "VALUES(?, ?, ?)";
  sql::Statement statement(
      GetDB().GetUniqueStatement(kInsertConversationQuery));
  CHECK(statement.is_valid());

  statement.BindString(0, conversation->uuid);

  BindAndEncryptOptionalString(statement, 1, conversation->title);
  BindOptionalString(statement, 2, conversation->model_key);

  if (!statement.Run()) {
    DVLOG(0) << "Failed to execute 'conversation' insert statement: "
             << db_.GetErrorMessage();
    return false;
  }

  if (conversation->associated_content->is_content_association_possible) {
    DVLOG(2) << "Adding associated content for conversation "
             << conversation->uuid << " with url "
             << conversation->associated_content->url->spec();
    if (!AddOrUpdateAssociatedContent(
            conversation->uuid, std::move(conversation->associated_content),
            contents)) {
      return false;
    }
  }

  if (!AddConversationEntry(conversation->uuid, std::move(first_entry))) {
    return false;
  }

  if (!transaction.Commit()) {
    DVLOG(0) << "Transaction commit failed with reason: "
             << db_.GetErrorMessage();
    return false;
  }

  return true;
}

bool AIChatDatabase::AddOrUpdateAssociatedContent(
    std::string_view conversation_uuid,
    mojom::SiteInfoPtr associated_content,
    std::optional<std::string> contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return false;
  }

  // TODO(petemill): handle multiple associated content per conversation
  CHECK(!conversation_uuid.empty());
  CHECK(associated_content->uuid.has_value());

  // Check if we already have persisted this content
  static constexpr char kSelectExistingAssociatedContentId[] =
      "SELECT uuid FROM associated_content WHERE conversation_uuid=?"
      " AND uuid=?";
  sql::Statement select_statement(GetDB().GetCachedStatement(
      SQL_FROM_HERE, kSelectExistingAssociatedContentId));
  CHECK(select_statement.is_valid());
  select_statement.BindString(0, conversation_uuid);
  select_statement.BindString(1, associated_content->uuid.value());

  sql::Statement statement;
  if (select_statement.Step()) {
    DVLOG(4) << "Updating associated content for conversation "
             << conversation_uuid << " with id "
             << associated_content->uuid.value();
    static constexpr char kUpdateAssociatedContentQuery[] =
        "UPDATE associated_content"
        " SET title = ?,"
        " url = ?,"
        " content_type = ?,"
        " last_contents = ?,"
        " content_used_percentage = ?,"
        " is_content_refined = ?"
        " WHERE uuid=? and conversation_uuid=?";
    statement.Assign(GetDB().GetUniqueStatement(kUpdateAssociatedContentQuery));
  } else {
    DVLOG(4) << "Inserting associated content for conversation "
             << conversation_uuid;
    static constexpr char kInsertAssociatedContentQuery[] =
        "INSERT INTO associated_content(title, url,"
        " content_type, last_contents, content_used_percentage,"
        " is_content_refined, uuid, conversation_uuid)"
        " VALUES(?, ?, ?, ?, ?, ?, ?, ?) ";
    statement.Assign(GetDB().GetUniqueStatement(kInsertAssociatedContentQuery));
  }
  CHECK(statement.is_valid());
  int index = 0;
  BindAndEncryptOptionalString(statement, index++, associated_content->title);
  BindAndEncryptOptionalString(statement, index++,
                               associated_content->url->spec());
  statement.BindInt(index++,
                    base::to_underlying(associated_content->content_type));
  BindAndEncryptOptionalString(statement, index++, contents);
  statement.BindInt(index++, associated_content->content_used_percentage);
  statement.BindBool(index++, associated_content->is_content_refined);
  statement.BindString(index++, associated_content->uuid.value());
  statement.BindString(index, conversation_uuid);

  if (!statement.Run()) {
    DVLOG(0)
        << "Failed to execute 'associated_content' insert or update statement: "
        << db_.GetErrorMessage();
    return false;
  }

  return true;
}

bool AIChatDatabase::AddConversationEntry(
    std::string_view conversation_uuid,
    mojom::ConversationTurnPtr entry,
    std::optional<std::string_view> model_key,
    std::optional<std::string> editing_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(!conversation_uuid.empty());
  CHECK(entry->uuid.has_value() && !entry->uuid->empty());
  if (!LazyInit()) {
    return false;
  }

  // Verify the conversation exists and get existing model key. We don't
  // want to add orphan conversation entries when the conversation doesn't
  // exist.
  static constexpr char kGetConversationIdQuery[] =
      "SELECT model_key FROM conversation WHERE uuid=?";
  sql::Statement get_conversation_model_statement(
      GetDB().GetCachedStatement(SQL_FROM_HERE, kGetConversationIdQuery));
  CHECK(get_conversation_model_statement.is_valid());
  get_conversation_model_statement.BindString(0, conversation_uuid);
  if (!get_conversation_model_statement.Step()) {
    DVLOG(0) << "ID not found in 'conversation' table";
    return false;
  }
  auto existing_model_key =
      GetOptionalString(get_conversation_model_statement, 0);

  sql::Transaction transaction(&GetDB());
  CHECK(GetDB().is_open());
  if (!transaction.Begin()) {
    DVLOG(0) << "Transaction cannot begin";
    return false;
  }

  bool has_valid_new_model_key = !model_key.value_or("").empty();
  bool should_update_model = (
      // Clear existing
      (!has_valid_new_model_key && existing_model_key.has_value()) ||
      // Change or add existing
      (has_valid_new_model_key &&
       (existing_model_key.value_or("") != model_key.value())));
  if (should_update_model) {
    // Update model key if neccessary
    static constexpr char kUpdateModelKeyQuery[] =
        "UPDATE conversation SET model_key=? WHERE uuid=?";
    sql::Statement update_model_key_statement(
        GetDB().GetCachedStatement(SQL_FROM_HERE, kUpdateModelKeyQuery));
    update_model_key_statement.BindString(1, conversation_uuid);
    if (has_valid_new_model_key) {
      update_model_key_statement.BindString(0, model_key.value());
    } else {
      update_model_key_statement.BindNull(0);
    }
    update_model_key_statement.Run();
  }

  sql::Statement insert_conversation_entry_statement;

  if (editing_id.has_value()) {
    static constexpr char kInsertEditingConversationEntryQuery[] =
        "INSERT INTO conversation_entry(editing_entry_uuid, uuid,"
        " conversation_uuid, date, entry_text, prompt,"
        " character_type, action_type, selected_text)"
        " VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)";
    insert_conversation_entry_statement.Assign(
        GetDB().GetUniqueStatement(kInsertEditingConversationEntryQuery));
  } else {
    static constexpr char kInsertConversationEntryQuery[] =
        "INSERT INTO conversation_entry(uuid, conversation_uuid, date,"
        " entry_text, prompt, character_type, action_type, selected_text)"
        " VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
    insert_conversation_entry_statement.Assign(
        GetDB().GetUniqueStatement(kInsertConversationEntryQuery));
  }
  CHECK(insert_conversation_entry_statement.is_valid());

  int index = 0;
  if (editing_id.has_value()) {
    insert_conversation_entry_statement.BindString(index++, editing_id.value());
  }
  insert_conversation_entry_statement.BindString(index++, entry->uuid.value());
  insert_conversation_entry_statement.BindString(index++, conversation_uuid);
  insert_conversation_entry_statement.BindTime(index++, entry->created_time);
  BindAndEncryptOptionalString(insert_conversation_entry_statement, index++,
                               entry->text);
  BindAndEncryptOptionalString(insert_conversation_entry_statement, index++,
                               entry->prompt);
  insert_conversation_entry_statement.BindInt(
      index++, base::to_underlying(entry->character_type));
  insert_conversation_entry_statement.BindInt(
      index++, base::to_underlying(entry->action_type));
  BindAndEncryptOptionalString(insert_conversation_entry_statement, index++,
                               entry->selected_text);

  if (!insert_conversation_entry_statement.Run()) {
    DVLOG(0) << "Failed to execute 'conversation_entry' insert statement: "
             << db_.GetErrorMessage();
    return false;
  }

  if (entry->events.has_value()) {
    for (size_t i = 0; i < entry->events->size(); i++) {
      const mojom::ConversationEntryEventPtr& event = entry->events->at(i);
      switch (event->which()) {
        case mojom::ConversationEntryEvent::Tag::kCompletionEvent: {
          sql::Statement event_statement(GetDB().GetCachedStatement(
              SQL_FROM_HERE,
              "INSERT INTO conversation_entry_event_completion"
              " (event_order, text, conversation_entry_uuid)"
              " VALUES(?, ?, ?)"));
          CHECK(event_statement.is_valid());
          event_statement.BindInt(0, static_cast<int>(i));
          if (!BindAndEncryptString(
                  event_statement, 1,
                  event->get_completion_event()->completion)) {
            return false;
          }
          event_statement.BindString(2, entry->uuid.value());
          event_statement.Run();
          break;
        }
        case mojom::ConversationEntryEvent::Tag::kSearchQueriesEvent: {
          sql::Statement event_statement(GetDB().GetCachedStatement(
              SQL_FROM_HERE,
              "INSERT INTO conversation_entry_event_search_queries"
              " (event_order, queries, conversation_entry_uuid)"
              " VALUES(?, ?, ?)"));
          CHECK(event_statement.is_valid());

          std::string queries_data = base::JoinString(
              event->get_search_queries_event()->search_queries,
              kSearchQueriesSeparator);

          event_statement.BindInt(0, static_cast<int>(i));
          if (!BindAndEncryptString(event_statement, 1, queries_data)) {
            return false;
          }
          event_statement.BindString(2, entry->uuid.value());
          event_statement.Run();
          break;
        }
        default: {
          break;
        }
      }
    }
  }

  if (entry->edits.has_value()) {
    for (auto& edit : entry->edits.value()) {
      if (!AddConversationEntry(conversation_uuid, std::move(edit), model_key,
                                entry->uuid.value())) {
        return false;
      }
    }
  }

  if (!transaction.Commit()) {
    DVLOG(0) << "Transaction commit failed with reason: "
             << db_.GetErrorMessage();
    return false;
  }

  return true;
}

bool AIChatDatabase::UpdateConversationTitle(std::string_view conversation_uuid,
                                             std::string_view title) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(4) << __func__ << " for " << conversation_uuid << " with title "
           << title;
  if (!LazyInit()) {
    return false;
  }

  static constexpr char kUpdateConversationTitleQuery[] =
      "UPDATE conversation SET title=? WHERE uuid=?";
  sql::Statement statement(
      GetDB().GetCachedStatement(SQL_FROM_HERE, kUpdateConversationTitleQuery));
  CHECK(statement.is_valid());

  if (!BindAndEncryptString(statement, 0, title)) {
    return false;
  }
  statement.BindString(1, conversation_uuid);

  return statement.Run();
}

bool AIChatDatabase::DeleteConversation(std::string_view conversation_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return false;
  }

  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    DVLOG(0) << "Transaction cannot begin\n";
    return false;
  }

  // Delete all conversation entries
  static constexpr char kSelectConversationEntryQuery[] =
      "SELECT uuid FROM conversation_entry WHERE conversation_uuid=?";
  sql::Statement select_conversation_entry_statement(
      GetDB().GetUniqueStatement(kSelectConversationEntryQuery));
  CHECK(select_conversation_entry_statement.is_valid());
  select_conversation_entry_statement.BindString(0, conversation_uuid);

  // Delete all conversation entry events
  while (select_conversation_entry_statement.Step()) {
    std::string conversation_entry_uuid =
        select_conversation_entry_statement.ColumnString(0);
    static constexpr char kDeleteCompletionEventQuery[] =
        "DELETE FROM conversation_entry_event_completion"
        " WHERE conversation_entry_uuid=?";
    sql::Statement delete_completion_event_statement(
        GetDB().GetUniqueStatement(kDeleteCompletionEventQuery));
    CHECK(delete_completion_event_statement.is_valid());
    delete_completion_event_statement.BindString(0, conversation_entry_uuid);
    if (!delete_completion_event_statement.Run()) {
      return false;
    }

    static constexpr char kDeleteSearchQueriesEventQuery[] =
        "DELETE FROM conversation_entry_event_search_queries "
        " WHERE conversation_entry_uuid=?";
    sql::Statement delete_queries_event_statement(
        GetDB().GetUniqueStatement(kDeleteSearchQueriesEventQuery));
    CHECK(delete_queries_event_statement.is_valid());
    delete_queries_event_statement.BindString(0, conversation_entry_uuid);
    if (!delete_queries_event_statement.Run()) {
      return false;
    }

    static constexpr char kDeleteEntryQuery[] =
        "DELETE FROM conversation_entry WHERE uuid=?";
    sql::Statement delete_conversation_entry_statement(
        GetDB().GetUniqueStatement(kDeleteEntryQuery));
    CHECK(delete_conversation_entry_statement.is_valid());
    delete_conversation_entry_statement.BindString(0, conversation_entry_uuid);
    if (!delete_conversation_entry_statement.Run()) {
      return false;
    }
  }

  // Delete the conversation metadata
  static constexpr char kDeleteAssociatedContentQuery[] =
      "DELETE FROM associated_content WHERE conversation_uuid=?";
  sql::Statement delete_associated_content_statement(
      GetDB().GetUniqueStatement(kDeleteAssociatedContentQuery));
  CHECK(delete_associated_content_statement.is_valid());
  delete_associated_content_statement.BindString(0, conversation_uuid);
  if (!delete_associated_content_statement.Run()) {
    return false;
  }

  static constexpr char kDeleteConversationQuery[] =
      "DELETE FROM conversation WHERE uuid=?";
  sql::Statement delete_conversation_statement(
      GetDB().GetUniqueStatement(kDeleteConversationQuery));
  CHECK(delete_conversation_statement.is_valid());
  delete_conversation_statement.BindString(0, conversation_uuid);
  if (!delete_conversation_statement.Run()) {
    return false;
  }

  if (!transaction.Commit()) {
    DVLOG(0) << "Transaction commit failed with reason: "
             << db_.GetErrorMessage();
    return false;
  }
  return true;
}

bool AIChatDatabase::DeleteConversationEntry(
    std::string_view conversation_entry_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return false;
  }

  sql::Transaction transaction(&db_);
  CHECK(!conversation_entry_uuid.empty());
  if (!transaction.Begin()) {
    DVLOG(0) << "Transaction cannot begin\n";
    return false;
  }
  // Delete from conversation_entry_event_completion
  {
    sql::Statement delete_statement(GetDB().GetUniqueStatement(
        "DELETE FROM conversation_entry_event_completion WHERE "
        "conversation_entry_uuid=?"));
    delete_statement.BindString(0, conversation_entry_uuid);
    if (!delete_statement.Run()) {
      DLOG(ERROR)
          << "Failed to delete from conversation_entry_event_completion "
             "for id: "
          << conversation_entry_uuid;
      return false;
    }
  }

  // Delete from conversation_entry_event_search_queries
  {
    static constexpr char kQuery[] =
        "DELETE FROM conversation_entry_event_search_queries WHERE "
        "conversation_entry_uuid=?";
    sql::Statement delete_statement(GetDB().GetUniqueStatement(kQuery));
    CHECK(delete_statement.is_valid());
    delete_statement.BindString(0, conversation_entry_uuid);
    if (!delete_statement.Run()) {
      DLOG(ERROR) << "Failed to delete from "
                     "conversation_entry_event_search_queries for conversation "
                     "entry uuid: "
                  << conversation_entry_uuid;
      return false;
    }
  }

  // Delete edits
  {
    static constexpr char kQuery[] =
        "DELETE FROM conversation_entry WHERE editing_entry_uuid = ?";
    sql::Statement delete_statement(GetDB().GetUniqueStatement(kQuery));
    CHECK(delete_statement.is_valid());
    delete_statement.BindString(0, conversation_entry_uuid);
    if (!delete_statement.Run()) {
      DLOG(ERROR) << "Failed to delete from conversation_entry for "
                     "conversation entry uuid: "
                  << conversation_entry_uuid;
      return false;
    }
  }

  // Delete from conversation_entry
  {
    static constexpr char kQuery[] =
        "DELETE FROM conversation_entry WHERE uuid=?";
    sql::Statement delete_statement(GetDB().GetUniqueStatement(kQuery));
    CHECK(delete_statement.is_valid());
    delete_statement.BindString(0, conversation_entry_uuid);
    if (!delete_statement.Run()) {
      LOG(ERROR) << "Failed to delete from conversation_entry for id: "
                 << conversation_entry_uuid;
      return false;
    }
  }

  if (!transaction.Commit()) {
    DVLOG(0) << "Transaction commit failed with reason: "
             << db_.GetErrorMessage();
    return false;
  }
  return true;
}

bool AIChatDatabase::DeleteAllData() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Ignore init failure when deletion. We only need the database to be open.
  LazyInit();

  if (!GetDB().is_open()) {
    return false;
  }

  // Delete everything
  if (!GetDB().Raze()) {
    return false;
  }

  // Re-init the database
  return LazyInit(true);
}

bool AIChatDatabase::DeleteAssociatedWebContent(
    std::optional<base::Time> begin_time,
    std::optional<base::Time> end_time) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return false;
  }
  DVLOG(4) << "Deleting associated web content for time range "
           << begin_time.value_or(base::Time()) << " to "
           << end_time.value_or(base::Time::Max());

  // Set any associated content url, title and content to NULL where
  // conversation had any entry between begin_time and end_time.
  static constexpr char kQuery[] =
      "UPDATE associated_content"
      " SET url=NULL, title=NULL, last_contents=NULL"
      " WHERE conversation_uuid IN ("
      "  SELECT conversation_uuid"
      "  FROM conversation_entry"
      "  WHERE date >= ? AND date <= ?)";
  sql::Statement statement(GetDB().GetUniqueStatement(kQuery));
  CHECK(statement.is_valid());
  statement.BindTime(0, begin_time.value_or(base::Time()));
  statement.BindTime(1, end_time.value_or(base::Time::Max()));
  if (!statement.Run()) {
    DVLOG(0) << "Failed to execute 'associated_content' update statement for "
                "DeleteAssociatedWebContent: "
             << db_.GetErrorMessage();
    return false;
  }
  return true;
}

sql::Database& AIChatDatabase::GetDB() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return db_;
}

std::string AIChatDatabase::DecryptColumnToString(sql::Statement& statement,
                                                  int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto decrypted_value = encryptor_.DecryptData(statement.ColumnBlob(index));
  if (!decrypted_value) {
    DVLOG(0) << "Failed to decrypt value";
    return "";
  }
  return *decrypted_value;
}

std::optional<std::string> AIChatDatabase::DecryptOptionalColumnToString(
    sql::Statement& statement,
    int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Don't allow non-BLOB types
  if (statement.GetColumnType(index) != sql::ColumnType::kBlob) {
    return std::nullopt;
  }
  auto decrypted_value = encryptor_.DecryptData(statement.ColumnBlob(index));
  if (!decrypted_value) {
    DVLOG(0) << "Failed to decrypt value";
    return std::nullopt;
  }
  return *decrypted_value;
}

void AIChatDatabase::BindAndEncryptOptionalString(
    sql::Statement& statement,
    int index,
    std::optional<std::string_view> value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (value.has_value() && !value.value().empty()) {
    auto encrypted_value = encryptor_.EncryptString(std::string(value.value()));
    if (!encrypted_value) {
      DVLOG(0) << "Failed to encrypt value";
      statement.BindNull(index);
      return;
    }
    statement.BindBlob(index, *encrypted_value);
  } else {
    statement.BindNull(index);
  }
}

bool AIChatDatabase::BindAndEncryptString(sql::Statement& statement,
                                          int index,
                                          std::string_view value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto encrypted_value = encryptor_.EncryptString(std::string(value));
  if (!encrypted_value) {
    DVLOG(0) << "Failed to encrypt value";
    return false;
  }
  statement.BindBlob(index, *encrypted_value);
  return true;
}

bool AIChatDatabase::CreateSchema() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  static constexpr char kCreateConversationTableQuery[] =
      "CREATE TABLE IF NOT EXISTS conversation("
      "uuid TEXT PRIMARY KEY NOT NULL,"
      // Encrypted conversation title string
      "title BLOB,"
      "model_key TEXT)";
  CHECK(GetDB().IsSQLValid(kCreateConversationTableQuery));
  if (!GetDB().Execute(kCreateConversationTableQuery)) {
    return false;
  }

  // AssociatedContent is 1:many with Conversation for future-proofing when
  // we support multiple associated contents per conversation.
  static constexpr char kCreateAssociatedContentTableQuery[] =
      "CREATE TABLE IF NOT EXISTS associated_content("
      "uuid TEXT PRIMARY KEY NOT NULL,"
      "conversation_uuid TEXT NOT NULL,"
      // Encrypted associated content title string
      "title BLOB,"
      // Encrypted url string
      "url BLOB,"
      // Stores SiteInfo.IsVideo. Future-proofed for multiple content types
      // 0 for regular content
      // 1 for video.
      "content_type INTEGER NOT NULL,"
      // Encrypted string value of the content, so that conversations can be
      // continued.
      "last_contents BLOB,"
      // Don't need REAL for content_used_percentage since
      // we're never using decimal values.
      // UI expects 0 - 100 values.
      "content_used_percentage INTEGER NOT NULL,"
      "is_content_refined INTEGER NOT NULL)";
  CHECK(GetDB().IsSQLValid(kCreateAssociatedContentTableQuery));
  if (!GetDB().Execute(kCreateAssociatedContentTableQuery)) {
    return false;
  }

  // AKA ConversationTurn in mojom
  static constexpr char kCreateConversationEntryTableQuery[] =
      "CREATE TABLE IF NOT EXISTS conversation_entry("
      "uuid TEXT PRIMARY KEY NOT NULL,"
      "conversation_uuid STRING NOT NULL,"
      "date INTEGER NOT NULL,"
      // Encrypted text string
      // TODO(petemill): move to event only
      "entry_text BLOB,"
      // Encrypted optional user-invisible override prompt
      "prompt BLOB,"
      "character_type INTEGER NOT NULL,"
      // editing_entry points to the ConversationEntry row that is being edited.
      // Edits can be sorted by date.
      "editing_entry_uuid TEXT,"
      "action_type INTEGER,"
      // Encrypted selected text
      "selected_text BLOB)";
  // TODO(petemill): Forking can be achieved by associating each
  // ConversationEntry with a parent ConversationEntry.
  // TODO(petemill): Store a model name with each entry to know when
  // a model was changed for a conversation, or for forking-by-model features.
  CHECK(GetDB().IsSQLValid(kCreateConversationEntryTableQuery));
  if (!GetDB().Execute(kCreateConversationEntryTableQuery)) {
    return false;
  }

  static constexpr char kCreateConversationEntryTextTableQuery[] =
      "CREATE TABLE IF NOT EXISTS conversation_entry_event_completion("
      "conversation_entry_uuid INTEGER NOT NULL,"
      "event_order INTEGER NOT NULL,"
      // encrypted event text string
      "text BLOB NOT NULL,"
      "PRIMARY KEY(conversation_entry_uuid, event_order)"
      ")";
  CHECK(GetDB().IsSQLValid(kCreateConversationEntryTextTableQuery));
  if (!GetDB().Execute(kCreateConversationEntryTextTableQuery)) {
    return false;
  }

  static constexpr char kCreateSearchQueriesTableQuery[] =
      "CREATE TABLE IF NOT EXISTS conversation_entry_event_search_queries("
      "conversation_entry_uuid INTEGER NOT NULL,"
      "event_order INTEGER NOT NULL,"
      // encrypted delimited search query strings
      "queries BLOB NOT NULL,"
      "PRIMARY KEY(conversation_entry_uuid, event_order)"
      ")";
  CHECK(GetDB().IsSQLValid(kCreateSearchQueriesTableQuery));
  if (!GetDB().Execute(kCreateSearchQueriesTableQuery)) {
    return false;
  }

  return true;
}

}  // namespace ai_chat
