/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_database.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/proto/store.pb.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/statement_id.h"
#include "sql/transaction.h"

namespace ai_chat {

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

bool MigrateFrom2To3(sql::Database* db) {
  static constexpr char kAddTotalTokenColumnQuery[] =
      "ALTER TABLE conversation ADD COLUMN total_tokens INTEGER DEFAULT 0";
  static constexpr char kAddTrimmedTokenColumnQuery[] =
      "ALTER TABLE conversation ADD COLUMN trimmed_tokens INTEGER DEFAULT 0";
  sql::Statement total_tokens_statement(
      db->GetUniqueStatement(kAddTotalTokenColumnQuery));
  sql::Statement trimmed_tokens_statement(
      db->GetUniqueStatement(kAddTrimmedTokenColumnQuery));
  return total_tokens_statement.is_valid() &&
         trimmed_tokens_statement.is_valid() && total_tokens_statement.Run() &&
         trimmed_tokens_statement.Run();
}

bool MigrateFrom3to4(sql::Database* db) {
  static constexpr char kAddTypeColumnQuery[] =
      "ALTER TABLE conversation_entry_uploaded_files ADD COLUMN type INTEGER "
      "DEFAULT 0";
  sql::Statement statement(db->GetUniqueStatement(kAddTypeColumnQuery));

  return statement.is_valid() && statement.Run();
}

bool MigrateFrom4to5(sql::Database* db) {
  static constexpr char kAddModelKeyColumnQuery[] =
      "ALTER TABLE conversation_entry ADD COLUMN model_key TEXT DEFAULT NULL";
  sql::Statement statement(db->GetUniqueStatement(kAddModelKeyColumnQuery));

  return statement.is_valid() && statement.Run();
}

bool MigrateFrom5to6(sql::Database* db) {
  static constexpr char kRemoveIsContentRefinedColumnQuery[] =
      "ALTER TABLE associated_content DROP COLUMN is_content_refined";
  sql::Statement statement(
      db->GetUniqueStatement(kRemoveIsContentRefinedColumnQuery));

  return statement.is_valid() && statement.Run();
}
void SerializeWebSourcesEvent(const mojom::WebSourcesEventPtr& mojom_event,
                              store::WebSourcesEventProto* proto_event) {
  proto_event->clear_sources();

  for (const auto& mojom_source : mojom_event->sources) {
    if (!mojom_source->url.is_valid() ||
        !mojom_source->favicon_url.is_valid()) {
      DVLOG(0) << "Invalid WebSourcesEvent found for persistence, with url: "
               << mojom_source->url.spec()
               << " and favicon url: " << mojom_source->favicon_url.spec();
      continue;
    }
    store::WebSourceProto* proto_source = proto_event->add_sources();
    proto_source->set_title(mojom_source->title);
    proto_source->set_url(mojom_source->url.spec());
    proto_source->set_favicon_url(mojom_source->favicon_url.spec());
  }
}

mojom::WebSourcesEventPtr DeserializeWebSourcesEvent(
    const store::WebSourcesEventProto& proto_event) {
  auto mojom_event = mojom::WebSourcesEvent::New();
  mojom_event->sources.reserve(proto_event.sources_size());

  for (const auto& proto_source : proto_event.sources()) {
    auto mojom_source = mojom::WebSource::New();
    mojom_source->title = proto_source.title();
    mojom_source->url = GURL(proto_source.url());
    if (!mojom_source->url.is_valid()) {
      DVLOG(0) << "Invalid WebSourcesEvent found in database with url: "
               << proto_source.url();
      continue;
    }
    mojom_source->favicon_url = GURL(proto_source.favicon_url());
    if (!mojom_source->favicon_url.is_valid()) {
      DVLOG(0) << "Invalid WebSourcesEvent found in database with favicon url: "
               << proto_source.favicon_url();
      continue;
    }
    mojom_event->sources.push_back(std::move(mojom_source));
  }
  return mojom_event;
}

}  // namespace

// These database versions should roll together unless we develop migrations.
// Lowest version we support migrations from - existing database will be deleted
// if lower.
constexpr int kLowestSupportedDatabaseVersion = 1;

// The oldest version of the schema such that a legacy Brave client using that
// version can still read/write the current database.
constexpr int kCompatibleDatabaseVersionNumber = 6;

// Current version of the database. Increase if breaking changes are made.
constexpr int kCurrentDatabaseVersion = 6;

AIChatDatabase::AIChatDatabase(const base::FilePath& db_file_path,
                               os_crypt_async::Encryptor encryptor)
    : db_file_path_(db_file_path),
      db_(sql::DatabaseOptions().set_page_size(4096).set_cache_size(1000),
          sql::Database::Tag("AIChatDatabase")),
      encryptor_(std::move(encryptor)) {
  base::AssertLongCPUWorkAllowed();
}

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
    DVLOG(0) << "Failed to open database at " << db_file_path_.value();
    return sql::InitStatus::INIT_FAILURE;
  }

  if (sql::MetaTable::RazeIfIncompatible(
          &GetDB(), kLowestSupportedDatabaseVersion, kCurrentDatabaseVersion) ==
      sql::RazeIfIncompatibleResult::kFailed) {
    DVLOG(0) << "Failed to raze incompatible database";
    return sql::InitStatus::INIT_FAILURE;
  }

  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    DVLOG(0) << "Failed to begin transaction: " << GetDB().GetErrorMessage();
    return sql::InitStatus::INIT_FAILURE;
  }

  sql::MetaTable meta_table;
  if (!meta_table.Init(&GetDB(), kCurrentDatabaseVersion,
                       kCompatibleDatabaseVersionNumber)) {
    DVLOG(0) << "Failed to init meta table";
    return sql::InitStatus::INIT_FAILURE;
  }

  if (meta_table.GetCompatibleVersionNumber() > kCurrentDatabaseVersion) {
    DVLOG(0) << "AIChat database version is too new.";
    return sql::InitStatus::INIT_TOO_NEW;
  }

  if (!CreateSchema()) {
    DVLOG(0) << "Failure to create tables";
    return sql::InitStatus::INIT_FAILURE;
  }

  if (meta_table.GetVersionNumber() < kCurrentDatabaseVersion) {
    bool migration_success = true;
    int current_version = meta_table.GetVersionNumber();
    if (current_version == 1) {
      migration_success = MigrateFrom1To2(&GetDB());
      if (migration_success) {
        migration_success = meta_table.SetCompatibleVersionNumber(
                                kCompatibleDatabaseVersionNumber) &&
                            meta_table.SetVersionNumber(2);
      }
      current_version = 2;
    }
    if (migration_success && current_version == 2) {
      migration_success = MigrateFrom2To3(&GetDB());
      if (migration_success) {
        migration_success = meta_table.SetCompatibleVersionNumber(
                                kCompatibleDatabaseVersionNumber) &&
                            meta_table.SetVersionNumber(3);
      }
      current_version = 3;
    }
    if (migration_success && current_version == 3) {
      migration_success = MigrateFrom3to4(&GetDB());
      if (migration_success) {
        migration_success = meta_table.SetCompatibleVersionNumber(
                                kCompatibleDatabaseVersionNumber) &&
                            meta_table.SetVersionNumber(4);
      }
      current_version = 4;
    }
    if (migration_success && current_version == 4) {
      migration_success = MigrateFrom4to5(&GetDB());
      if (migration_success) {
        migration_success = meta_table.SetCompatibleVersionNumber(
                                kCompatibleDatabaseVersionNumber) &&
                            meta_table.SetVersionNumber(5);
      }
      current_version = 5;
    }
    if (migration_success && current_version == 5) {
      migration_success = MigrateFrom5to6(&GetDB());
      if (migration_success) {
        migration_success = meta_table.SetCompatibleVersionNumber(
                                kCompatibleDatabaseVersionNumber) &&
                            meta_table.SetVersionNumber(6);
      }
      current_version = 6;
    }

    // Migration unsuccessful, raze the database and re-init
    if (!migration_success) {
      if (db_.Raze()) {
        return InitInternal();
      }
      DVLOG(0) << "Init failure after unsuccessful migration and raze";
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
      "  conversation.total_tokens, conversation.trimmed_tokens,"
      "  last_activity_date.date,"
      "  associated_content.uuid, associated_content.title,"
      "  associated_content.url, associated_content.content_type,"
      "  associated_content.content_used_percentage"
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
    if (conversation && conversation->uuid != uuid) {
      conversation_list.emplace_back(std::move(conversation));
    }

    if (!conversation) {
      conversation = mojom::Conversation::New();
    }

    auto index = 1;

    conversation->uuid = uuid;
    conversation->title =
        DecryptOptionalColumnToString(statement, index++).value_or("");
    conversation->model_key = GetOptionalString(statement, index++);
    conversation->total_tokens = statement.ColumnInt(index++);
    conversation->trimmed_tokens = statement.ColumnInt(index++);
    conversation->updated_time = statement.ColumnTime(index++);
    conversation->has_content = true;

    if (statement.GetColumnType(index) != sql::ColumnType::kNull) {
      DVLOG(1) << __func__ << " got associated content";
      auto associated_content = mojom::AssociatedContent::New();
      associated_content->uuid = statement.ColumnString(index++);
      associated_content->title =
          DecryptOptionalColumnToString(statement, index++).value_or("");
      auto url_raw = DecryptOptionalColumnToString(statement, index++);
      if (url_raw.has_value()) {
        associated_content->url = GURL(url_raw.value());
      }
      associated_content->content_type =
          static_cast<mojom::ContentType>(statement.ColumnInt(index++));
      associated_content->content_used_percentage =
          statement.ColumnInt(index++);

      conversation->associated_content.push_back(std::move(associated_content));
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
      "action_type, selected_text, model_key"
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
    auto model_key = GetOptionalString(statement, index++);

    auto entry = mojom::ConversationTurn::New(
        entry_uuid, character_type, action_type, text, prompt, selected_text,
        std::nullopt, date, std::nullopt, std::nullopt, false, model_key);

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

    // Web Source events
    {
      sql::Statement event_statement(GetDB().GetUniqueStatement(
          "SELECT event_order, sources_serialized"
          " FROM conversation_entry_event_web_sources"
          " WHERE conversation_entry_uuid=?"
          " ORDER BY event_order ASC"));
      event_statement.BindString(0, entry_uuid);

      while (event_statement.Step()) {
        int event_order = event_statement.ColumnInt(0);
        auto data = DecryptColumnToString(event_statement, 1);
        store::WebSourcesEventProto proto_event;
        if (proto_event.ParseFromString(data)) {
          mojom::WebSourcesEventPtr mojom_event =
              DeserializeWebSourcesEvent(proto_event);
          if (mojom_event->sources.empty()) {
            DVLOG(0) << "Empty WebSourcesEvent found in database for entry "
                     << entry_uuid;
            continue;
          }
          events.emplace_back(
              Event{event_order, mojom::ConversationEntryEvent::NewSourcesEvent(
                                     std::move(mojom_event))});
        }
      }
    }

    // insert events in order
    if (!events.empty()) {
      std::ranges::sort(events, [](const Event& a, const Event& b) {
        return a.event_order < b.event_order;
      });
      entry->events = std::vector<mojom::ConversationEntryEventPtr>{};
      for (auto& event : events) {
        entry->events->emplace_back(std::move(event.event));
      }
    }

    // Uploaded files
    sql::Statement uploaded_file_statement(
        GetDB().GetUniqueStatement("SELECT filename, filesize, data, type"
                                   " FROM conversation_entry_uploaded_files"
                                   " WHERE conversation_entry_uuid=?"
                                   " ORDER BY file_order ASC"));
    uploaded_file_statement.BindString(0, entry_uuid);

    while (uploaded_file_statement.Step()) {
      auto filename = DecryptColumnToString(uploaded_file_statement, 0);
      int64_t filesize = uploaded_file_statement.ColumnInt64(1);
      auto decrypted_bytes_str =
          DecryptColumnToString(uploaded_file_statement, 2);
      base::span<const uint8_t> raw_bytes =
          base::as_byte_span(decrypted_bytes_str);
      std::vector<uint8_t> data(raw_bytes.begin(), raw_bytes.end());
      auto type = static_cast<mojom::UploadedFileType>(
          uploaded_file_statement.ColumnInt(3));
      if (!entry->uploaded_files) {
        entry->uploaded_files = std::vector<mojom::UploadedFilePtr>{};
      }
      entry->uploaded_files->emplace_back(mojom::UploadedFile::New(
          std::move(filename), filesize, std::move(data), type));
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

  while (statement.Step()) {
    auto content = mojom::ContentArchive::New(
        statement.ColumnString(0), DecryptColumnToString(statement, 1));
    archive_contents.emplace_back(std::move(content));
  }
  return archive_contents;
}

bool AIChatDatabase::AddConversation(mojom::ConversationPtr conversation,
                                     std::vector<std::string> contents,
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
      "INSERT INTO conversation(uuid, title, model_key, total_tokens, "
      "trimmed_tokens) "
      "VALUES(?, ?, ?, ?, ?)";
  sql::Statement statement(
      GetDB().GetUniqueStatement(kInsertConversationQuery));
  CHECK(statement.is_valid());

  statement.BindString(0, conversation->uuid);

  BindAndEncryptOptionalString(statement, 1, conversation->title);
  BindOptionalString(statement, 2, conversation->model_key);
  statement.BindInt64(3, conversation->total_tokens);
  statement.BindInt64(4, conversation->trimmed_tokens);

  if (!statement.Run()) {
    DVLOG(0) << "Failed to execute 'conversation' insert statement: "
             << db_.GetErrorMessage();
    return false;
  }

  if (!conversation->associated_content.empty()) {
    DVLOG(2) << "Adding associated content for conversation "
             << conversation->uuid << " with urls "
             << std::accumulate(conversation->associated_content.begin(),
                                conversation->associated_content.end(),
                                std::string(),
                                [](const auto& a, const auto& b) {
                                  return a + b->url.spec() + ", ";
                                });
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
    std::vector<mojom::AssociatedContentPtr> associated_content,
    std::vector<std::string> contents) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK_EQ(associated_content.size(), contents.size());

  if (!LazyInit()) {
    return false;
  }

  // Note: This needs to run inside a transaction so its safe to bail out if
  // inserting/updating one AssociatedContent fails.
  sql::Transaction transaction(&GetDB());
  if (!transaction.Begin()) {
    DVLOG(0) << "Transaction cannot begin";
    return false;
  }

  CHECK(!conversation_uuid.empty());
  CHECK(!associated_content.empty());

  // Check which content ids already exist for this conversation.
  base::flat_set<std::string> existing_ids_set;
  {
    static constexpr char kSelectExistingAssociatedContentIds[] =
        "SELECT uuid FROM associated_content WHERE conversation_uuid=?";
    sql::Statement select_existing_ids(GetDB().GetCachedStatement(
        SQL_FROM_HERE, kSelectExistingAssociatedContentIds));
    select_existing_ids.BindString(0, conversation_uuid);
    std::vector<std::string> existing_ids;
    while (select_existing_ids.Step()) {
      existing_ids.push_back(select_existing_ids.ColumnString(0));
    }

    // Store as a set for faster lookup. Note: We don't push directly to the set
    // as that is O(n**2) with a base::flat_set.
    existing_ids_set = existing_ids;
  }

  for (size_t i = 0; i < associated_content.size(); ++i) {
    auto& content = associated_content[i];
    auto content_text = contents.empty() ? "" : std::move(contents[i]);

    sql::Statement insert_or_update_statement;
    bool exists = existing_ids_set.contains(content->uuid);
    if (exists) {
      DVLOG(4) << "Updating associated content for conversation "
               << conversation_uuid << " with id " << content->uuid;
      static constexpr char kUpdateAssociatedContentQuery[] =
          "UPDATE associated_content"
          " SET title = ?,"
          " url = ?,"
          " content_type = ?,"
          " last_contents = ?,"
          " content_used_percentage = ?"
          " WHERE uuid=? and conversation_uuid=?";
      insert_or_update_statement.Assign(
          GetDB().GetUniqueStatement(kUpdateAssociatedContentQuery));
    } else {
      DVLOG(4) << "Inserting associated content for conversation "
               << conversation_uuid;
      static constexpr char kInsertAssociatedContentQuery[] =
          "INSERT INTO associated_content(title, url,"
          " content_type, last_contents, content_used_percentage,"
          " uuid, conversation_uuid)"
          " VALUES(?, ?, ?, ?, ?, ?, ?) ";
      insert_or_update_statement.Assign(
          GetDB().GetUniqueStatement(kInsertAssociatedContentQuery));
    }

    CHECK(insert_or_update_statement.is_valid());
    int index = 0;
    BindAndEncryptOptionalString(insert_or_update_statement, index++,
                                 content->title);
    BindAndEncryptOptionalString(insert_or_update_statement, index++,
                                 content->url.spec());
    insert_or_update_statement.BindInt(
        index++, base::to_underlying(content->content_type));
    BindAndEncryptOptionalString(insert_or_update_statement, index++,
                                 content_text);
    insert_or_update_statement.BindInt(index++,
                                       content->content_used_percentage);
    insert_or_update_statement.BindString(index++, content->uuid);
    insert_or_update_statement.BindString(index, conversation_uuid);

    if (!insert_or_update_statement.Run()) {
      DVLOG(0) << "Failed to execute 'associated_content' insert or update "
                  "statement: "
               << db_.GetErrorMessage();
      // Note: This should run inside a transaction, so its safe to bail out
      // here.
      transaction.Rollback();
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
        " character_type, action_type, selected_text, model_key)"
        " VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    insert_conversation_entry_statement.Assign(
        GetDB().GetUniqueStatement(kInsertEditingConversationEntryQuery));
  } else {
    static constexpr char kInsertConversationEntryQuery[] =
        "INSERT INTO conversation_entry(uuid, conversation_uuid, date,"
        " entry_text, prompt, character_type, action_type, selected_text,"
        " model_key)"
        " VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)";
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
  BindOptionalString(insert_conversation_entry_statement, index++,
                     entry->model_key);

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
        case mojom::ConversationEntryEvent::Tag::kSourcesEvent: {
          sql::Statement event_statement(GetDB().GetCachedStatement(
              SQL_FROM_HERE,
              "INSERT INTO conversation_entry_event_web_sources"
              " (event_order, sources_serialized, conversation_entry_uuid)"
              " VALUES(?, ?, ?)"));
          CHECK(event_statement.is_valid());

          store::WebSourcesEventProto proto_event;
          SerializeWebSourcesEvent(event->get_sources_event(), &proto_event);
          if (proto_event.sources().empty()) {
            DVLOG(0) << "Empty WebSourcesEvent found for persistence";
            break;
          }
          event_statement.BindInt(0, static_cast<int>(i));
          if (!BindAndEncryptString(event_statement, 1,
                                    proto_event.SerializeAsString())) {
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

  if (entry->uploaded_files.has_value()) {
    for (size_t i = 0; i < entry->uploaded_files->size(); ++i) {
      const mojom::UploadedFilePtr& uploaded_file =
          entry->uploaded_files->at(i);
      sql::Statement uploaded_file_statement(GetDB().GetCachedStatement(
          SQL_FROM_HERE,
          "INSERT INTO conversation_entry_uploaded_files"
          "(file_order, filename, filesize, data, type,"
          " conversation_entry_uuid)"
          " VALUES(?, ?, ?, ?, ?, ?)"));
      CHECK(uploaded_file_statement.is_valid());
      uploaded_file_statement.BindInt(0, static_cast<int>(i));
      if (!BindAndEncryptString(uploaded_file_statement, 1,
                                uploaded_file->filename)) {
        return false;
      }
      uploaded_file_statement.BindInt64(2, uploaded_file->filesize);
      if (!BindAndEncryptString(
              uploaded_file_statement, 3,
              base::as_string_view(base::span(uploaded_file->data)))) {
        return false;
      }
      uploaded_file_statement.BindInt(4, static_cast<int>(uploaded_file->type));
      uploaded_file_statement.BindString(5, entry->uuid.value());
      uploaded_file_statement.Run();
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

bool AIChatDatabase::UpdateConversationTokenInfo(
    std::string_view conversation_uuid,
    uint64_t total_tokens,
    uint64_t trimmed_tokens) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(4) << __func__ << " for " << conversation_uuid << " with total_tokens "
           << total_tokens << " and trimed_tokens " << trimmed_tokens;
  if (!LazyInit()) {
    return false;
  }

  static constexpr char kUpdateConversationTokenInfoQuery[] =
      "UPDATE conversation SET total_tokens=?, trimmed_tokens=? WHERE uuid=?";
  sql::Statement statement(GetDB().GetCachedStatement(
      SQL_FROM_HERE, kUpdateConversationTokenInfoQuery));
  CHECK(statement.is_valid());

  statement.BindInt64(0, total_tokens);
  statement.BindInt64(1, trimmed_tokens);
  statement.BindString(2, conversation_uuid);

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

    static constexpr char kDeleteWebSourcesEventQuery[] =
        "DELETE FROM conversation_entry_event_web_sources "
        " WHERE conversation_entry_uuid=?";
    sql::Statement delete_web_sources_event_statement(
        GetDB().GetUniqueStatement(kDeleteWebSourcesEventQuery));
    CHECK(delete_web_sources_event_statement.is_valid());
    delete_web_sources_event_statement.BindString(0, conversation_entry_uuid);
    if (!delete_web_sources_event_statement.Run()) {
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

    static constexpr char kDeleteUploadedFilesQuery[] =
        "DELETE FROM conversation_entry_uploaded_files "
        " WHERE conversation_entry_uuid=?";
    sql::Statement delete_uploaded_images_statement(
        GetDB().GetUniqueStatement(kDeleteUploadedFilesQuery));
    CHECK(delete_uploaded_images_statement.is_valid());
    delete_uploaded_images_statement.BindString(0, conversation_entry_uuid);
    if (!delete_uploaded_images_statement.Run()) {
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

  // Delete from conversation_entry_event_web_sources
  {
    static constexpr char kQuery[] =
        "DELETE FROM conversation_entry_event_web_sources WHERE "
        "conversation_entry_uuid=?";
    sql::Statement delete_statement(GetDB().GetUniqueStatement(kQuery));
    CHECK(delete_statement.is_valid());
    delete_statement.BindString(0, conversation_entry_uuid);
    if (!delete_statement.Run()) {
      DLOG(ERROR) << "Failed to delete from "
                     "conversation_entry_event_web_sources for conversation "
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

  // Delete from conversation_entry_uploaded_files
  {
    static constexpr char kQuery[] =
        "DELETE FROM conversation_entry_uploaded_files WHERE "
        "conversation_entry_uuid=?";
    sql::Statement delete_statement(GetDB().GetUniqueStatement(kQuery));
    CHECK(delete_statement.is_valid());
    delete_statement.BindString(0, conversation_entry_uuid);
    if (!delete_statement.Run()) {
      DLOG(ERROR) << "Failed to delete from "
                     "conversation_entry_uploaded_files for conversation "
                     "entry uuid: "
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
      DLOG(ERROR) << "Failed to delete from conversation_entry for id: "
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
      "model_key TEXT,"
      "total_tokens INTEGER NOT NULL,"
      "trimmed_tokens INTEGER NOT NULL)";
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
      // Stores AssociatedContent.IsVideo. Future-proofed for multiple content
      // types 0 for regular content 1 for video.
      "content_type INTEGER NOT NULL,"
      // Encrypted string value of the content, so that conversations can be
      // continued.
      "last_contents BLOB,"
      // Don't need REAL for content_used_percentage since
      // we're never using decimal values.
      // UI expects 0 - 100 values.
      "content_used_percentage INTEGER NOT NULL)";
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
      "selected_text BLOB,"
      "model_key TEXT)";
  // TODO(petemill): Forking can be achieved by associating each
  // ConversationEntry with a parent ConversationEntry.
  // TODO(petemill): Store a model name with each entry to know when
  // a model was changed for a conversation, or for forking-by-model features.
  CHECK(GetDB().IsSQLValid(kCreateConversationEntryTableQuery));
  if (!GetDB().Execute(kCreateConversationEntryTableQuery)) {
    return false;
  }

  // TODO(petemill): Consider storing all conversation entry events in a single
  // table, with serialized data in protocol buffers format. If we need to add
  // search capability for the encrypted data, we could store some generic
  // embeddings in a separate table or column.

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

  static constexpr char kCreateWebSourcesTableQuery[] =
      "CREATE TABLE IF NOT EXISTS conversation_entry_event_web_sources("
      "conversation_entry_uuid INTEGER NOT NULL,"
      "event_order INTEGER NOT NULL,"
      // encrypted serialized web source data
      "sources_serialized BLOB NOT NULL,"
      "PRIMARY KEY(conversation_entry_uuid, event_order)"
      ")";
  CHECK(GetDB().IsSQLValid(kCreateWebSourcesTableQuery));
  if (!GetDB().Execute(kCreateWebSourcesTableQuery)) {
    return false;
  }

  static constexpr char kCreateUploadedFilesTableQuery[] =
      "CREATE TABLE IF NOT EXISTS conversation_entry_uploaded_files("
      "conversation_entry_uuid INTEGER NOT NULL,"
      "file_order INTEGER NOT NULL,"
      // encrypted filename
      "filename BLOB NOT NULL,"
      "filesize INTEGER NOT NULL,"
      // encrypted file byte data
      "data BLOB NOT NULL,"
      // mojom::UploadedFileType
      "type INTEGER NOT NULL,"
      "PRIMARY KEY(conversation_entry_uuid, file_order)"
      ")";
  CHECK(GetDB().IsSQLValid(kCreateUploadedFilesTableQuery));
  if (!GetDB().Execute(kCreateUploadedFilesTableQuery)) {
    return false;
  }

  return true;
}

}  // namespace ai_chat
