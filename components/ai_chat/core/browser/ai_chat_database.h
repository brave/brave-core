/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_DATABASE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_DATABASE_H_

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/local_ai/buildflags/buildflags.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "components/sync/model/sync_metadata_store.h"
#include "sql/database.h"
#include "sql/init_status.h"

#if BUILDFLAG(ENABLE_LOCAL_AI)
#include "brave/components/ai_chat/core/browser/learned_memory_types.h"
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

namespace sql {
class Statement;
}  // namespace sql

namespace syncer {
class MetadataBatch;
}  // namespace syncer

namespace ai_chat {

extern const int kLowestSupportedDatabaseVersion;
extern const int kCompatibleDatabaseVersionNumber;
extern const int kCurrentDatabaseVersion;

// Identifies a conversation entry whose associated web content was cleared by
// DeleteAssociatedWebContent(), so callers can propagate the change (e.g.
// re-sync the affected entries).
struct ClearedAssociatedContentEntry {
  std::string conversation_uuid;
  std::string entry_uuid;
};

// Persists AI Chat conversations and associated content. Conversations are
// mainly formed of their conversation entries. Edits to conversation entries
// should be handled with removal and re-adding so that other classes can make
// decisions about how it affects the rest of history.
// All data should be stored encrypted.
class AIChatDatabase : public syncer::SyncMetadataStore {
 public:
  AIChatDatabase(const base::FilePath& db_file_path,
                 scoped_refptr<os_crypt_async::Encryptor> encryptor);
  AIChatDatabase(const AIChatDatabase&) = delete;
  AIChatDatabase& operator=(const AIChatDatabase&) = delete;
  ~AIChatDatabase() override;

  // Gets lightweight metadata for all conversations. No high-memory-consuming
  // data is returned.
  virtual std::vector<mojom::ConversationPtr> GetAllConversations();

  // Gets all data needed to rehydrate a conversation
  virtual mojom::ConversationArchivePtr GetConversationData(
      std::string_view conversation_uuid);

  // Gets all conversation entries belonging to the thread with the provided
  // uuid.
  virtual std::vector<mojom::ConversationTurnPtr> GetConversationThreadEntries(
      std::string_view thread_uuid);

  // Gets every entry of the conversation with the provided uuid, including the
  // entries of its threads, in creation order. Unlike GetConversationData(),
  // neither thread metadata nor associated content is read.
  virtual std::vector<mojom::ConversationTurnPtr> GetAllConversationEntries(
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

  // Adds a new thread's metadata to the database. A no-op if the thread's
  // uuid already exists.
  virtual bool AddConversationThread(mojom::ThreadPtr thread);

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

  // Updates the token information of the thread with the provided UUID
  virtual bool UpdateThreadTokenInfo(std::string_view thread_uuid,
                                     uint64_t total_tokens,
                                     uint64_t trimmed_tokens);

  // Deletes the conversation with the provided UUID, and the learned memory
  // data that came from it (see DeleteMemoryDataFromSource()).
  virtual bool DeleteConversation(std::string_view conversation_uuid);

  // Deletes the conversation entry with the provided ID and all associated
  // edits and events, and the learned memory data that came from them.
  virtual bool DeleteConversationEntry(
      std::string_view conversation_entry_uuid);

#if BUILDFLAG(ENABLE_LOCAL_AI)
  // Gets all learned memories, with their source links and previous texts.
  virtual std::vector<LearnedMemory> GetAllLearnedMemories();

  // The uuid and the text version of every learned memory. Decrypts nothing.
  virtual std::vector<LearnedMemoryStamp> GetLearnedMemoryStamps();

  // Gets the learned memories with the given uuids, in the same order. The
  // memories have no source links and no previous text. A uuid that is not
  // stored is left out.
  virtual std::vector<LearnedMemory> GetLearnedMemoriesByUuid(
      const std::vector<std::string>& memory_uuids);

  // Gets the learned memories of the permanent type, with the same content as
  // GetLearnedMemoriesByUuid().
  virtual std::vector<LearnedMemory> GetPermanentLearnedMemories();

  // Adds the memory, or replaces the stored memory with the same uuid,
  // including its source links and previous text. The database sets the text
  // version: 1 for a new memory, the stored version + 1 when the text differs
  // from the stored text, else the stored version.
  virtual bool AddOrUpdateLearnedMemory(const LearnedMemory& memory);

  // Deletes the memory with its source links and its previous text. Nothing
  // remembers the memory afterwards: Dreaming can learn it again.
  virtual bool DeleteLearnedMemory(std::string_view memory_uuid);

  // The watermark of a conversation is the date of the last user turn that
  // Dreaming processed. Conversations without a watermark are not in the map.
  virtual std::map<std::string, base::Time> GetAllMemoryWatermarks();
  virtual bool SetMemoryWatermark(std::string_view conversation_uuid,
                                  base::Time last_processed_entry_date);
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

  // Drops all data and tables in the database, and re-creates empty tables.
  // This includes the learned memory tables, which exist in all builds.
  virtual bool DeleteAllData();

  // Clears (sets to NULL) the url/title/last_contents of associated content for
  // every conversation that has an entry dated within [begin_time, end_time].
  // Returns the entries whose content was actually cleared, so callers can
  // propagate the change; returns std::nullopt if the operation failed.
  virtual std::optional<std::vector<ClearedAssociatedContentEntry>>
  DeleteAssociatedWebContent(std::optional<base::Time> begin_time,
                             std::optional<base::Time> end_time);

  // Reads all sync metadata (entity metadata + data type state) into the batch.
  bool GetAllSyncMetadata(syncer::MetadataBatch* metadata_batch);

  // Deletes all entity metadata (but not data type state).
  bool ClearAllEntityMetadata();

  // syncer::SyncMetadataStore:
  bool UpdateEntityMetadata(syncer::DataType data_type,
                            const std::string& storage_key,
                            const sync_pb::EntityMetadata& metadata) override;
  bool ClearEntityMetadata(syncer::DataType data_type,
                           const std::string& storage_key) override;
  bool UpdateDataTypeState(
      syncer::DataType data_type,
      const sync_pb::DataTypeState& data_type_state) override;
  bool ClearDataTypeState(syncer::DataType data_type) override;

 private:
  friend class AIChatDatabaseTest;
  friend class AIChatDatabaseMigrationTest;
  FRIEND_TEST_ALL_PREFIXES(AIChatDatabaseTest, ConversationThreadEntries);

  // What a row in the memory_source_link table belongs to. The values are
  // stored in the database. Do not reorder or reuse them.
  enum class MemoryLinkOwner {
    kMemoryText = 0,
    kMemoryPreviousText = 1,
    // 2 was the owner kind of a tombstone in an earlier version. Do not reuse
    // it.
  };

  sql::Database& GetDB();

#if BUILDFLAG(ENABLE_LOCAL_AI)
  // Reads the rows of a statement that selects the columns of
  // kLightLearnedMemoryColumns (see the .cc file).
  std::vector<LearnedMemory> ReadLightLearnedMemories(
      sql::Statement& statement);
#endif

  // Initializes the database if it hasn't been initialized yet. If |re_init|
  // is true, it will forget previous intiialization state and attempt to
  // re-initialize the database (e.g. after a table deletion).
  bool LazyInit(bool re_init = false);
  sql::InitStatus InitInternal();

  std::vector<mojom::ConversationTurnPtr> GetConversationEntries(
      sql::Statement& statement);
  std::vector<mojom::ThreadPtr> GetConversationThreads(
      std::string_view conversation_uuid);
  std::vector<mojom::ContentArchivePtr> GetArchiveContentsForConversation(
      std::string_view conversation_uuid);

  bool GetAllEntityMetadata(syncer::MetadataBatch* metadata_batch);
  bool GetDataTypeState(sync_pb::DataTypeState* state);

#if BUILDFLAG(ENABLE_LOCAL_AI)
  std::vector<MemorySourceLink> GetMemorySourceLinks(
      std::string_view owner_uuid,
      MemoryLinkOwner owner);
  bool ReplaceMemorySourceLinks(std::string_view owner_uuid,
                                MemoryLinkOwner owner,
                                const std::vector<MemorySourceLink>& links);
#endif  // BUILDFLAG(ENABLE_LOCAL_AI)

  // Removes what came from the conversation or the conversation entry: the
  // links, the memories with no link left in their current text, the previous
  // texts with a link to it, and the tombstone links. Tombstones stay, so that
  // a deleted chat does not make Dreaming learn a deleted memory again. The
  // caller must have a transaction open.
  bool DeleteMemoryDataFromConversation(std::string_view conversation_uuid);
  bool DeleteMemoryDataFromEntry(std::string_view entry_uuid);
  bool DeleteMemoryDataFromSource(std::string_view link_column,
                                  std::string_view uuid);

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
  scoped_refptr<os_crypt_async::Encryptor> encryptor_
      GUARDED_BY_CONTEXT(sequence_checker_);
  // The initialization status of the database. It's not set if never attempted.
  std::optional<sql::InitStatus> db_init_status_ = std::nullopt;

  // Verifies that all operations happen on the same sequence.
  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_DATABASE_H_
