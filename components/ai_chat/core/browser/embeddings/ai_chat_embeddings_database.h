// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_AI_CHAT_EMBEDDINGS_DATABASE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_AI_CHAT_EMBEDDINGS_DATABASE_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "base/time/time.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "sql/database.h"
#include "sql/init_status.h"

namespace passage_embeddings {
class Embedding;
}  // namespace passage_embeddings

namespace ai_chat {

inline constexpr base::FilePath::CharType kAIChatEmbeddingsDatabaseFileName[] =
    FILE_PATH_LITERAL("AIChatEmbeddings");

// A passage of a conversation entry, or of its title, with its embedding.
struct ConversationPassage {
  ConversationPassage();
  ConversationPassage(std::string entry_uuid,
                      std::optional<std::string> thread_uuid,
                      base::Time created_time,
                      std::string text,
                      std::vector<float> embedding);
  ConversationPassage(ConversationPassage&&);
  ConversationPassage& operator=(ConversationPassage&&);
  ~ConversationPassage();

  // Empty for a passage of the conversation's title.
  std::string entry_uuid;
  std::optional<std::string> thread_uuid;
  base::Time created_time;
  std::string text;
  std::vector<float> embedding;
};

// A passage found by AIChatEmbeddingsDatabase::SearchConversations().
struct ConversationPassageMatch {
  ConversationPassageMatch();
  ConversationPassageMatch(ConversationPassageMatch&&);
  ConversationPassageMatch& operator=(ConversationPassageMatch&&);
  ~ConversationPassageMatch();

  std::string entry_uuid;
  std::optional<std::string> thread_uuid;
  base::Time created_time;
  std::string text;
  float score = 0.0f;
};

// A conversation found by AIChatEmbeddingsDatabase::SearchConversations().
struct ConversationMatch {
  ConversationMatch();
  ConversationMatch(ConversationMatch&&);
  ConversationMatch& operator=(ConversationMatch&&);
  ~ConversationMatch();

  std::string conversation_uuid;
  // The score of the conversation's best passage.
  float score = 0.0f;
  // Best first.
  std::vector<ConversationPassageMatch> passages;
};

// A memory with the embedding of its text.
struct MemoryPassage {
  std::string text;
  std::vector<float> embedding;
};

// A memory found by AIChatEmbeddingsDatabase::SearchMemories().
struct MemoryMatch {
  std::string text;
  float score = 0.0f;
};

// SQLite store for the embeddings of the user's Leo conversations and
// memories. Both the text and the embedding of each passage are encrypted,
// since an embedding can give away the text it was computed from. Lives on a
// background sequence, typically through base::SequenceBound.
class AIChatEmbeddingsDatabase {
 public:
  // Stored embeddings are deleted when they were not computed by the model at
  // `model_version`, or from passages prepared at `passage_version`, since
  // they couldn't be compared with new ones.
  AIChatEmbeddingsDatabase(const base::FilePath& db_file_path,
                           int64_t model_version,
                           int passage_version,
                           scoped_refptr<os_crypt_async::Encryptor> encryptor);
  AIChatEmbeddingsDatabase(const AIChatEmbeddingsDatabase&) = delete;
  AIChatEmbeddingsDatabase& operator=(const AIChatEmbeddingsDatabase&) = delete;
  ~AIChatEmbeddingsDatabase();

  // The conversations that have stored passages, each with the time they are
  // indexed up to. A null time means the conversation has to be indexed
  // again.
  base::flat_map<std::string, base::Time> GetIndexedConversations();

  // Replaces the passages of the entry with `entry_uuid`, whose uuid each of
  // `passages` carries, and moves the conversation's indexed time forward to
  // `indexed_time`, if given.
  bool ReplaceEntryPassages(std::string_view conversation_uuid,
                            std::string_view entry_uuid,
                            std::vector<ConversationPassage> passages,
                            std::optional<base::Time> indexed_time);

  // Replaces all of the conversation's passages, and records that it is
  // indexed up to `indexed_time`.
  bool ReplaceConversationPassages(std::string_view conversation_uuid,
                                   std::vector<ConversationPassage> passages,
                                   base::Time indexed_time);

  // Records that the conversation has to be indexed again.
  bool InvalidateConversation(std::string_view conversation_uuid);

  bool DeleteEntryPassages(std::string_view conversation_uuid,
                           std::string_view entry_uuid);
  bool DeleteConversation(std::string_view conversation_uuid);
  bool DeleteAllConversations();

  // Finds up to `count` conversations, ranked by their best passage, with up
  // to `max_passages` of their passages each. Passages scoring below
  // `min_score` against `query` are ignored, as are those of the conversation
  // with `excluded_conversation_uuid`.
  std::vector<ConversationMatch> SearchConversations(
      const passage_embeddings::Embedding& query,
      float min_score,
      size_t count,
      size_t max_passages,
      std::string_view excluded_conversation_uuid);

  // Deletes the stored memories that are not in `memories`, and returns those
  // of `memories` that have no stored embedding.
  std::vector<std::string> SyncMemories(
      const std::vector<std::string>& memories);
  bool AddMemories(std::vector<MemoryPassage> memories);
  bool DeleteAllMemories();

  // Finds up to `count` memories scoring at least `min_score` against
  // `query`, best first.
  std::vector<MemoryMatch> SearchMemories(
      const passage_embeddings::Embedding& query,
      float min_score,
      size_t count);

 private:
  bool LazyInit();
  sql::InitStatus InitInternal();

  bool InsertPassages(std::string_view conversation_uuid,
                      base::span<const ConversationPassage> passages);
  bool DeletePassages(std::string_view conversation_uuid,
                      std::optional<std::string_view> entry_uuid);

  std::optional<std::vector<uint8_t>> EncryptEmbedding(
      base::span<const float> embedding);
  std::optional<std::vector<float>> DecryptEmbedding(sql::Statement& statement,
                                                     int column);
  std::optional<std::string> DecryptText(sql::Statement& statement, int column);

  const base::FilePath db_file_path_;
  const int64_t model_version_;
  const int passage_version_;
  const scoped_refptr<os_crypt_async::Encryptor> encryptor_;

  sql::Database db_ GUARDED_BY_CONTEXT(sequence_checker_);
  std::optional<sql::InitStatus> init_status_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_EMBEDDINGS_AI_CHAT_EMBEDDINGS_DATABASE_H_
