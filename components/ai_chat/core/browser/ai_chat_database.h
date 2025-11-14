/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_DATABASE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_DATABASE_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "sql/database.h"
#include "sql/init_status.h"

namespace ai_chat {

extern const int kLowestSupportedDatabaseVersion;
extern const int kCompatibleDatabaseVersionNumber;
extern const int kCurrentDatabaseVersion;

// Persists AI Chat conversations and associated content. Conversations are
// mainly formed of their conversation entries. Edits to conversation entries
// should be handled with removal and re-adding so that other classes can make
// decisions about how it affects the rest of history.
// All data should be stored encrypted.
class AIChatDatabase {
 public:
  AIChatDatabase(const base::FilePath& db_file_path,
                 os_crypt_async::Encryptor encryptor);
  AIChatDatabase(const AIChatDatabase&) = delete;
  AIChatDatabase& operator=(const AIChatDatabase&) = delete;
  virtual ~AIChatDatabase();

  // Gets lightweight metadata for all conversations. No high-memory-consuming
  // data is returned.
  virtual std::vector<mojom::ConversationPtr> GetAllConversations();

  // Gets all data needed to rehydrate a conversation
  virtual mojom::ConversationArchivePtr GetConversationData(
      std::string_view conversation_uuid);

  // Returns new ID for the provided entry and any provided associated content
  virtual bool AddConversation(mojom::ConversationPtr conversation,
                               std::vector<std::string> contents,
                               mojom::ConversationTurnPtr first_entry);

  // Update any properties of associated content metadata or full-text content
  virtual bool AddOrUpdateAssociatedContent(
      std::string_view conversation_uuid,
      std::vector<mojom::AssociatedContentPtr> associated_content,
      std::vector<std::string> contents);

  // Adds a new conversation entry to the conversation with the provided UUID
  virtual bool AddConversationEntry(
      std::string_view conversation_uuid,
      mojom::ConversationTurnPtr entry,
      std::optional<std::string> editing_id = std::nullopt);

  virtual bool UpdateToolUseEvent(std::string_view entry_uuid,
                                  size_t event_order,
                                  mojom::ToolUseEventPtr tool_use_event);

  // Updates the title of the conversation with the provided UUID
  virtual bool UpdateConversationTitle(std::string_view conversation_uuid,
                                       std::string_view title);

  // Updates the model of the conversation with the provided value
  virtual bool UpdateConversationModelKey(std::string_view conversation_uuid,
                                          std::optional<std::string> model_key);

  // Updates the token information of the conversation with the provided UUID
  virtual bool UpdateConversationTokenInfo(std::string_view conversation_uuid,
                                           uint64_t total_tokens,
                                           uint64_t trimmed_tokens);

  // Updates the NEAR verification status of a conversation entry
  virtual bool UpdateEntryVerificationStatus(std::string_view entry_uuid,
                                             bool verified);

  // Deletes the conversation with the provided UUID
  virtual bool DeleteConversation(std::string_view conversation_uuid);

  // Deletes the conversation entry with the provided ID and all associated
  // edits and events.
  virtual bool DeleteConversationEntry(
      std::string_view conversation_entry_uuid);

  // Drops all data and tables in the database, and re-creates empty tables
  virtual bool DeleteAllData();

  virtual bool DeleteAssociatedWebContent(std::optional<base::Time> begin_time,
                                          std::optional<base::Time> end_time);

 private:
  friend class AIChatDatabaseTest;
  friend class AIChatDatabaseMigrationTest;

  sql::Database& GetDB();

  // Initializes the database if it hasn't been initialized yet. If |re_init|
  // is true, it will forget previous intiialization state and attempt to
  // re-initialize the database (e.g. after a table deletion).
  bool LazyInit(bool re_init = false);
  sql::InitStatus InitInternal();

  std::vector<mojom::ConversationTurnPtr> GetConversationEntries(
      std::string_view conversation_id);
  std::vector<mojom::ContentArchivePtr> GetArchiveContentsForConversation(
      std::string_view conversation_uuid);

  std::string DecryptColumnToString(sql::Statement& statement, int index);
  std::optional<std::string> DecryptOptionalColumnToString(
      sql::Statement& statement,
      int index);
  void BindAndEncryptOptionalString(sql::Statement& statement,
                                    int index,
                                    std::optional<std::string_view> value);
  bool BindAndEncryptString(sql::Statement& statement,
                            int index,
                            std::string_view value);

  bool CreateSchema();

  // The directory storing the database.
  const base::FilePath db_file_path_;

  // The underlying SQL database
  sql::Database db_ GUARDED_BY_CONTEXT(sequence_checker_);
  os_crypt_async::Encryptor encryptor_ GUARDED_BY_CONTEXT(sequence_checker_);
  // The initialization status of the database. It's not set if never attempted.
  std::optional<sql::InitStatus> db_init_status_ = std::nullopt;

  // Verifies that all operations happen on the same sequence.
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_DATABASE_H_
