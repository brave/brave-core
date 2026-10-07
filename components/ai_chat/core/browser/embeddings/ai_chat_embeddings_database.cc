// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_database.h"

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <numeric>
#include <utility>

#include "base/check.h"
#include "base/containers/flat_set.h"
#include "base/numerics/byte_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

namespace ai_chat {

namespace {

constexpr int kCurrentVersion = 1;

constexpr char kModelVersionKey[] = "model_version";
constexpr char kPassageVersionKey[] = "passage_version";

constexpr char kCreateConversationTable[] =
    "CREATE TABLE IF NOT EXISTS conversation("
    "uuid TEXT PRIMARY KEY NOT NULL,"
    // The time the conversation's passages are indexed up to, compared with
    // the conversation's last update. Zero when it has to be indexed again.
    "indexed_time INTEGER NOT NULL)";

constexpr char kCreateConversationPassageTable[] =
    "CREATE TABLE IF NOT EXISTS conversation_passage("
    "conversation_uuid TEXT NOT NULL,"
    // Empty for passages of the conversation's title.
    "entry_uuid TEXT NOT NULL,"
    "thread_uuid TEXT,"
    "passage_index INTEGER NOT NULL,"
    "created_time INTEGER NOT NULL,"
    // The text and the embedding are encrypted separately, so that a search
    // only decrypts the text of the passages it returns.
    "text BLOB NOT NULL,"
    "embedding BLOB NOT NULL,"
    "UNIQUE(conversation_uuid, entry_uuid, passage_index))";

constexpr char kCreateMemoryPassageTable[] =
    "CREATE TABLE IF NOT EXISTS memory_passage("
    "text BLOB NOT NULL,"
    "embedding BLOB NOT NULL)";

constexpr char kCreateLearnedMemoryPassageTable[] =
    "CREATE TABLE IF NOT EXISTS learned_memory_passage("
    // The uuid of the learned memory in the conversation database, which keeps
    // its text.
    "memory_uuid TEXT PRIMARY KEY NOT NULL,"
    // The text version the embedding was computed from.
    "text_version INTEGER NOT NULL,"
    "embedding BLOB NOT NULL)";

// Stored embeddings are unit vectors, so their dot product is their cosine
// similarity.
float DotProduct(base::span<const float> a, base::span<const float> b) {
  return std::inner_product(a.begin(), a.end(), b.begin(), 0.0f);
}

}  // namespace

ConversationPassage::ConversationPassage() = default;
ConversationPassage::ConversationPassage(std::string entry_uuid,
                                         std::optional<std::string> thread_uuid,
                                         base::Time created_time,
                                         std::string text,
                                         std::vector<float> embedding)
    : entry_uuid(std::move(entry_uuid)),
      thread_uuid(std::move(thread_uuid)),
      created_time(created_time),
      text(std::move(text)),
      embedding(std::move(embedding)) {}
ConversationPassage::ConversationPassage(ConversationPassage&&) = default;
ConversationPassage& ConversationPassage::operator=(ConversationPassage&&) =
    default;
ConversationPassage::~ConversationPassage() = default;

ConversationPassageMatch::ConversationPassageMatch() = default;
ConversationPassageMatch::ConversationPassageMatch(ConversationPassageMatch&&) =
    default;
ConversationPassageMatch& ConversationPassageMatch::operator=(
    ConversationPassageMatch&&) = default;
ConversationPassageMatch::~ConversationPassageMatch() = default;

ConversationMatch::ConversationMatch() = default;
ConversationMatch::ConversationMatch(ConversationMatch&&) = default;
ConversationMatch& ConversationMatch::operator=(ConversationMatch&&) = default;
ConversationMatch::~ConversationMatch() = default;

AIChatEmbeddingsDatabase::AIChatEmbeddingsDatabase(
    const base::FilePath& db_file_path,
    int64_t model_version,
    int passage_version,
    scoped_refptr<os_crypt_async::Encryptor> encryptor)
    : db_file_path_(db_file_path),
      model_version_(model_version),
      passage_version_(passage_version),
      encryptor_(std::move(encryptor)),
      db_(sql::DatabaseOptions(), sql::Database::Tag("AIChatEmbeddings")) {
  CHECK(encryptor_);
  // Constructed on the sequence that owns it, then used on its own.
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AIChatEmbeddingsDatabase::~AIChatEmbeddingsDatabase() = default;

base::flat_map<std::string, base::Time>
AIChatEmbeddingsDatabase::GetIndexedConversations() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return {};
  }
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE, "SELECT uuid, indexed_time FROM conversation"));
  std::vector<std::pair<std::string, base::Time>> conversations;
  while (statement.Step()) {
    conversations.emplace_back(statement.ColumnString(0),
                               statement.ColumnTime(1));
  }
  return base::flat_map<std::string, base::Time>(std::move(conversations));
}

bool AIChatEmbeddingsDatabase::ReplaceEntryPassages(
    std::string_view conversation_uuid,
    std::string_view entry_uuid,
    std::vector<ConversationPassage> passages,
    std::optional<base::Time> indexed_time) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(std::ranges::all_of(passages, [&](const ConversationPassage& passage) {
    return passage.entry_uuid == entry_uuid;
  }));
  if (!LazyInit()) {
    return false;
  }
  sql::Transaction transaction(&db_);
  if (!transaction.Begin() || !DeletePassages(conversation_uuid, entry_uuid) ||
      !InsertPassages(conversation_uuid, passages)) {
    return false;
  }
  sql::Statement insert(db_.GetCachedStatement(
      SQL_FROM_HERE,
      "INSERT OR IGNORE INTO conversation(uuid, indexed_time) VALUES(?, ?)"));
  insert.BindString(0, conversation_uuid);
  insert.BindTime(1, base::Time());
  if (!insert.Run()) {
    return false;
  }
  if (indexed_time) {
    sql::Statement update(db_.GetCachedStatement(
        SQL_FROM_HERE,
        "UPDATE conversation SET indexed_time=MAX(indexed_time, ?)"
        " WHERE uuid=?"));
    update.BindTime(0, *indexed_time);
    update.BindString(1, conversation_uuid);
    if (!update.Run()) {
      return false;
    }
  }
  return transaction.Commit();
}

bool AIChatEmbeddingsDatabase::ReplaceConversationPassages(
    std::string_view conversation_uuid,
    std::vector<ConversationPassage> passages,
    base::Time indexed_time) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return false;
  }
  sql::Transaction transaction(&db_);
  if (!transaction.Begin() ||
      !DeletePassages(conversation_uuid, std::nullopt) ||
      !InsertPassages(conversation_uuid, passages)) {
    return false;
  }
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE,
      "INSERT OR REPLACE INTO conversation(uuid, indexed_time) VALUES(?, ?)"));
  statement.BindString(0, conversation_uuid);
  statement.BindTime(1, indexed_time);
  return statement.Run() && transaction.Commit();
}

bool AIChatEmbeddingsDatabase::InvalidateConversation(
    std::string_view conversation_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return false;
  }
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE, "UPDATE conversation SET indexed_time=? WHERE uuid=?"));
  statement.BindTime(0, base::Time());
  statement.BindString(1, conversation_uuid);
  return statement.Run();
}

bool AIChatEmbeddingsDatabase::DeleteEntryPassages(
    std::string_view conversation_uuid,
    std::string_view entry_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return LazyInit() && DeletePassages(conversation_uuid, entry_uuid);
}

bool AIChatEmbeddingsDatabase::DeleteConversation(
    std::string_view conversation_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return false;
  }
  sql::Transaction transaction(&db_);
  if (!transaction.Begin() ||
      !DeletePassages(conversation_uuid, std::nullopt)) {
    return false;
  }
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE, "DELETE FROM conversation WHERE uuid=?"));
  statement.BindString(0, conversation_uuid);
  return statement.Run() && transaction.Commit();
}

bool AIChatEmbeddingsDatabase::DeleteAllConversations() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return false;
  }
  sql::Transaction transaction(&db_);
  return transaction.Begin() &&
         db_.Execute("DELETE FROM conversation_passage") &&
         db_.Execute("DELETE FROM conversation") && transaction.Commit();
}

std::vector<ConversationMatch> AIChatEmbeddingsDatabase::SearchConversations(
    const passage_embeddings::Embedding& query,
    float min_score,
    size_t count,
    size_t max_passages,
    std::string_view excluded_conversation_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit() || count == 0 || max_passages == 0) {
    return {};
  }

  struct Candidate {
    int64_t rowid;
    float score;
  };
  // Scores every passage, decrypting only the embeddings.
  std::map<std::string, std::vector<Candidate>> candidates;
  {
    sql::Statement statement(db_.GetCachedStatement(
        SQL_FROM_HERE,
        "SELECT rowid, conversation_uuid, embedding FROM conversation_passage"
        " WHERE conversation_uuid!=?"));
    statement.BindString(0, excluded_conversation_uuid);
    while (statement.Step()) {
      std::optional<std::vector<float>> embedding =
          DecryptEmbedding(statement, 2);
      if (!embedding || embedding->size() != query.GetData().size()) {
        continue;
      }
      const float score = DotProduct(*embedding, query.GetData());
      if (score < min_score) {
        continue;
      }
      candidates[statement.ColumnString(1)].push_back(
          {statement.ColumnInt64(0), score});
    }
  }

  std::vector<ConversationMatch> matches;
  for (auto& [conversation_uuid, passages] : candidates) {
    std::ranges::sort(passages, std::greater<>(), &Candidate::score);
    passages.resize(std::min(passages.size(), max_passages));
    ConversationMatch match;
    match.conversation_uuid = conversation_uuid;
    match.score = passages.front().score;
    matches.push_back(std::move(match));
  }
  std::ranges::sort(matches, std::greater<>(), &ConversationMatch::score);
  matches.resize(std::min(matches.size(), count));

  // Reads the text of the passages being returned.
  for (ConversationMatch& match : matches) {
    for (const Candidate& candidate : candidates.at(match.conversation_uuid)) {
      sql::Statement statement(db_.GetCachedStatement(
          SQL_FROM_HERE,
          "SELECT entry_uuid, thread_uuid, created_time, text"
          " FROM conversation_passage WHERE rowid=?"));
      statement.BindInt64(0, candidate.rowid);
      if (!statement.Step()) {
        continue;
      }
      std::optional<std::string> text = DecryptText(statement, 3);
      if (!text) {
        continue;
      }
      ConversationPassageMatch passage;
      passage.entry_uuid = statement.ColumnString(0);
      if (statement.GetColumnType(1) != sql::ColumnType::kNull) {
        passage.thread_uuid = statement.ColumnString(1);
      }
      passage.created_time = statement.ColumnTime(2);
      passage.text = std::move(*text);
      passage.score = candidate.score;
      match.passages.push_back(std::move(passage));
    }
  }
  std::erase_if(matches, [](const ConversationMatch& match) {
    return match.passages.empty();
  });
  return matches;
}

std::vector<std::string> AIChatEmbeddingsDatabase::SyncMemories(
    const std::vector<std::string>& memories) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return {};
  }
  const base::flat_set<std::string_view> wanted(memories.begin(),
                                                memories.end());
  base::flat_set<std::string> stored;
  std::vector<int64_t> stale_rowids;
  {
    sql::Statement statement(db_.GetCachedStatement(
        SQL_FROM_HERE, "SELECT rowid, text FROM memory_passage"));
    while (statement.Step()) {
      std::optional<std::string> text = DecryptText(statement, 1);
      if (!text || !wanted.contains(*text) || stored.contains(*text)) {
        stale_rowids.push_back(statement.ColumnInt64(0));
        continue;
      }
      stored.insert(std::move(*text));
    }
  }

  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    return {};
  }
  for (int64_t rowid : stale_rowids) {
    sql::Statement statement(db_.GetCachedStatement(
        SQL_FROM_HERE, "DELETE FROM memory_passage WHERE rowid=?"));
    statement.BindInt64(0, rowid);
    if (!statement.Run()) {
      return {};
    }
  }
  if (!transaction.Commit()) {
    return {};
  }

  std::vector<std::string> missing;
  for (const std::string& memory : memories) {
    if (stored.insert(memory).second) {
      missing.push_back(memory);
    }
  }
  return missing;
}

bool AIChatEmbeddingsDatabase::AddMemories(
    std::vector<MemoryPassage> memories) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return false;
  }
  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    return false;
  }
  for (const MemoryPassage& memory : memories) {
    std::optional<std::vector<uint8_t>> text =
        encryptor_->EncryptString(memory.text);
    std::optional<std::vector<uint8_t>> embedding =
        EncryptEmbedding(memory.embedding);
    if (!text || !embedding) {
      return false;
    }
    sql::Statement statement(db_.GetCachedStatement(
        SQL_FROM_HERE,
        "INSERT INTO memory_passage(text, embedding) VALUES(?, ?)"));
    statement.BindBlob(0, std::move(*text));
    statement.BindBlob(1, std::move(*embedding));
    if (!statement.Run()) {
      return false;
    }
  }
  return transaction.Commit();
}

bool AIChatEmbeddingsDatabase::DeleteAllMemories() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return LazyInit() && db_.Execute("DELETE FROM memory_passage");
}

std::vector<MemoryMatch> AIChatEmbeddingsDatabase::SearchMemories(
    const passage_embeddings::Embedding& query,
    float min_score,
    size_t count) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit() || count == 0) {
    return {};
  }
  std::vector<MemoryMatch> matches;
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE, "SELECT text, embedding FROM memory_passage"));
  while (statement.Step()) {
    std::optional<std::vector<float>> embedding =
        DecryptEmbedding(statement, 1);
    if (!embedding || embedding->size() != query.GetData().size()) {
      continue;
    }
    const float score = DotProduct(*embedding, query.GetData());
    if (score < min_score) {
      continue;
    }
    std::optional<std::string> text = DecryptText(statement, 0);
    if (!text) {
      continue;
    }
    matches.push_back({std::move(*text), score});
  }
  std::ranges::sort(matches, std::greater<>(), &MemoryMatch::score);
  matches.resize(std::min(matches.size(), count));
  return matches;
}

std::vector<std::string> AIChatEmbeddingsDatabase::SyncLearnedMemories(
    const std::vector<LearnedMemoryStamp>& memories) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return {};
  }
  const auto wanted = base::MakeFlatMap<std::string_view, int>(
      memories, {}, [](const LearnedMemoryStamp& memory) {
        return std::make_pair(std::string_view(memory.uuid),
                              memory.text_version);
      });
  base::flat_set<std::string> stored;
  std::vector<std::string> stale_uuids;
  {
    sql::Statement statement(db_.GetCachedStatement(
        SQL_FROM_HERE,
        "SELECT memory_uuid, text_version FROM learned_memory_passage"));
    while (statement.Step()) {
      std::string uuid = statement.ColumnString(0);
      auto it = wanted.find(uuid);
      if (it == wanted.end() || it->second != statement.ColumnInt(1)) {
        stale_uuids.push_back(std::move(uuid));
        continue;
      }
      stored.insert(std::move(uuid));
    }
  }

  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    return {};
  }
  for (const std::string& uuid : stale_uuids) {
    sql::Statement statement(db_.GetCachedStatement(
        SQL_FROM_HERE,
        "DELETE FROM learned_memory_passage WHERE memory_uuid=?"));
    statement.BindString(0, uuid);
    if (!statement.Run()) {
      return {};
    }
  }
  if (!transaction.Commit()) {
    return {};
  }

  std::vector<std::string> missing;
  for (const LearnedMemoryStamp& memory : memories) {
    if (stored.insert(memory.uuid).second) {
      missing.push_back(memory.uuid);
    }
  }
  return missing;
}

bool AIChatEmbeddingsDatabase::AddLearnedMemories(
    std::vector<LearnedMemoryPassage> memories) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit()) {
    return false;
  }
  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    return false;
  }
  for (const LearnedMemoryPassage& memory : memories) {
    std::optional<std::vector<uint8_t>> embedding =
        EncryptEmbedding(memory.embedding);
    if (!embedding) {
      return false;
    }
    sql::Statement statement(db_.GetCachedStatement(
        SQL_FROM_HERE,
        "INSERT OR REPLACE INTO learned_memory_passage(memory_uuid,"
        " text_version, embedding) VALUES(?, ?, ?)"));
    statement.BindString(0, memory.memory_uuid);
    statement.BindInt(1, memory.text_version);
    statement.BindBlob(2, std::move(*embedding));
    if (!statement.Run()) {
      return false;
    }
  }
  return transaction.Commit();
}

bool AIChatEmbeddingsDatabase::DeleteAllLearnedMemories() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return LazyInit() && db_.Execute("DELETE FROM learned_memory_passage");
}

std::vector<LearnedMemoryMatch> AIChatEmbeddingsDatabase::SearchLearnedMemories(
    const std::vector<std::vector<float>>& queries,
    size_t count) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!LazyInit() || count == 0 || queries.empty()) {
    return {};
  }
  std::vector<LearnedMemoryMatch> matches;
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE,
      "SELECT memory_uuid, embedding FROM learned_memory_passage"));
  while (statement.Step()) {
    std::optional<std::vector<float>> embedding =
        DecryptEmbedding(statement, 1);
    if (!embedding) {
      continue;
    }
    std::optional<float> best;
    for (const std::vector<float>& query : queries) {
      if (query.size() == embedding->size()) {
        const float score = DotProduct(*embedding, query);
        best = std::max(best.value_or(score), score);
      }
    }
    if (best) {
      matches.push_back({statement.ColumnString(0), *best});
    }
  }
  std::ranges::sort(matches, std::greater<>(), &LearnedMemoryMatch::score);
  matches.resize(std::min(matches.size(), count));
  return matches;
}

bool AIChatEmbeddingsDatabase::LazyInit() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!init_status_) {
    init_status_ = InitInternal();
  }
  return *init_status_ == sql::INIT_OK;
}

sql::InitStatus AIChatEmbeddingsDatabase::InitInternal() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::AssertLongCPUWorkAllowed();
  if (!db_.Open(db_file_path_)) {
    return sql::INIT_FAILURE;
  }
  if (sql::MetaTable::RazeIfIncompatible(&db_, kCurrentVersion,
                                         kCurrentVersion) ==
      sql::RazeIfIncompatibleResult::kFailed) {
    return sql::INIT_FAILURE;
  }

  sql::Transaction transaction(&db_);
  if (!transaction.Begin()) {
    return sql::INIT_FAILURE;
  }
  sql::MetaTable meta_table;
  if (!meta_table.Init(&db_, kCurrentVersion, kCurrentVersion) ||
      !db_.Execute(kCreateConversationTable) ||
      !db_.Execute(kCreateConversationPassageTable) ||
      !db_.Execute(kCreateMemoryPassageTable) ||
      !db_.Execute(kCreateLearnedMemoryPassageTable)) {
    return sql::INIT_FAILURE;
  }

  int64_t model_version = 0;
  int passage_version = 0;
  meta_table.GetValue(kModelVersionKey, &model_version);
  meta_table.GetValue(kPassageVersionKey, &passage_version);
  if (model_version != model_version_ || passage_version != passage_version_) {
    if (!db_.Execute("DELETE FROM conversation_passage") ||
        !db_.Execute("DELETE FROM conversation") ||
        !db_.Execute("DELETE FROM memory_passage") ||
        !db_.Execute("DELETE FROM learned_memory_passage") ||
        !meta_table.SetValue(kModelVersionKey, model_version_) ||
        !meta_table.SetValue(kPassageVersionKey, passage_version_)) {
      return sql::INIT_FAILURE;
    }
  }
  return transaction.Commit() ? sql::INIT_OK : sql::INIT_FAILURE;
}

bool AIChatEmbeddingsDatabase::InsertPassages(
    std::string_view conversation_uuid,
    base::span<const ConversationPassage> passages) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Each entry's passages are numbered in the order given.
  base::flat_map<std::string_view, int> next_passage_index;
  for (const ConversationPassage& passage : passages) {
    std::optional<std::vector<uint8_t>> text =
        encryptor_->EncryptString(passage.text);
    std::optional<std::vector<uint8_t>> embedding =
        EncryptEmbedding(passage.embedding);
    if (!text || !embedding) {
      return false;
    }
    sql::Statement statement(db_.GetCachedStatement(
        SQL_FROM_HERE,
        "INSERT INTO conversation_passage(conversation_uuid, entry_uuid,"
        " thread_uuid, passage_index, created_time, text, embedding)"
        " VALUES(?, ?, ?, ?, ?, ?, ?)"));
    statement.BindString(0, conversation_uuid);
    statement.BindString(1, passage.entry_uuid);
    if (passage.thread_uuid) {
      statement.BindString(2, *passage.thread_uuid);
    } else {
      statement.BindNull(2);
    }
    statement.BindInt(3, next_passage_index[passage.entry_uuid]++);
    statement.BindTime(4, passage.created_time);
    statement.BindBlob(5, std::move(*text));
    statement.BindBlob(6, std::move(*embedding));
    if (!statement.Run()) {
      return false;
    }
  }
  return true;
}

bool AIChatEmbeddingsDatabase::DeletePassages(
    std::string_view conversation_uuid,
    std::optional<std::string_view> entry_uuid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!entry_uuid) {
    sql::Statement statement(db_.GetCachedStatement(
        SQL_FROM_HERE,
        "DELETE FROM conversation_passage WHERE conversation_uuid=?"));
    statement.BindString(0, conversation_uuid);
    return statement.Run();
  }
  sql::Statement statement(
      db_.GetCachedStatement(SQL_FROM_HERE,
                             "DELETE FROM conversation_passage"
                             " WHERE conversation_uuid=? AND entry_uuid=?"));
  statement.BindString(0, conversation_uuid);
  statement.BindString(1, *entry_uuid);
  return statement.Run();
}

std::optional<std::vector<uint8_t>> AIChatEmbeddingsDatabase::EncryptEmbedding(
    base::span<const float> embedding) {
  std::string bytes;
  bytes.reserve(embedding.size() * sizeof(float));
  for (float value : embedding) {
    const std::array<uint8_t, 4u> value_bytes =
        base::FloatToLittleEndian(value);
    bytes.append(value_bytes.begin(), value_bytes.end());
  }
  return encryptor_->EncryptString(bytes);
}

std::optional<std::vector<float>> AIChatEmbeddingsDatabase::DecryptEmbedding(
    sql::Statement& statement,
    int column) {
  std::optional<std::string> bytes =
      encryptor_->DecryptData(statement.ColumnBlob(column));
  if (!bytes || bytes->empty() || bytes->size() % sizeof(float) != 0) {
    return std::nullopt;
  }
  const base::span<const uint8_t> data = base::as_byte_span(*bytes);
  std::vector<float> embedding(data.size() / sizeof(float));
  for (size_t i = 0; i < embedding.size(); ++i) {
    embedding[i] = base::FloatFromLittleEndian(
        data.subspan(i * sizeof(float)).first<sizeof(float)>());
  }
  return embedding;
}

std::optional<std::string> AIChatEmbeddingsDatabase::DecryptText(
    sql::Statement& statement,
    int column) {
  return encryptor_->DecryptData(statement.ColumnBlob(column));
}

}  // namespace ai_chat
