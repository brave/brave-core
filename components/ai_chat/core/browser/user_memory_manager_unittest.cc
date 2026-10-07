// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/user_memory_manager.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/threading/sequence_bound.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/dreaming_run.h"
#include "brave/components/ai_chat/core/browser/dreaming_text_utils.h"
#include "brave/components/ai_chat/core/browser/engine/mock_engine_consumer.h"
#include "brave/components/ai_chat/core/browser/learned_memory_data_source.h"
#include "brave/components/ai_chat/core/browser/learned_memory_eval.h"
#include "brave/components/ai_chat/core/browser/learned_memory_search.h"
#include "brave/components/ai_chat/core/browser/memory_decision_client.h"
#include "brave/components/ai_chat/core/browser/turn_memory_lookup.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/os_crypt/async/browser/test_utils.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

using testing::ElementsAre;
using testing::IsEmpty;

SentenceDecisions MakeDecisions(
    double fact_yes,
    SafetyAnswer safety,
    LearnedMemoryCategory category = LearnedMemoryCategory::kPersonalFact,
    double temporary_yes = 0.05) {
  SentenceDecisions decisions;
  decisions.fact = {{true, fact_yes}, {false, 1.0 - fact_yes}};
  for (SafetyAnswer answer :
       {SafetyAnswer::kOk, SafetyAnswer::kSensitive, SafetyAnswer::kInstruction,
        SafetyAnswer::kShortLived, SafetyAnswer::kNotAboutUser}) {
    decisions.safety[answer] = answer == safety ? 0.94 : 0.01;
  }
  decisions.category[category] = 0.9;
  decisions.temporary = {{true, temporary_yes}, {false, 1.0 - temporary_yes}};
  return decisions;
}

class FakeDecisionClient : public MemoryDecisionClient {
 public:
  void AskGate(const std::string& turn_text, GateCallback callback) override {
    gate_requests.push_back(turn_text);
    if (on_gate_asked) {
      std::move(on_gate_asked).Run();
    }
    if (hold_replies) {
      return;
    }
    std::optional<AnswerProbabilities<bool>> answer;
    if (!fail_gate) {
      double yes = gate_yes.contains(turn_text) ? 0.96 : 0.03;
      if (gate_unsure.contains(turn_text)) {
        yes = 0.1;
      }
      answer = AnswerProbabilities<bool>{{true, yes}, {false, 1.0 - yes}};
    }
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::move(answer)));
  }

  void AskSentenceDecisions(std::vector<std::string> sentences,
                            SentenceDecisionsCallback callback) override {
    std::vector<SentenceDecisions> answers;
    for (const auto& sentence : sentences) {
      sentence_requests.push_back(sentence);
      auto it = sentence_decisions.find(sentence);
      answers.push_back(it != sentence_decisions.end()
                            ? it->second
                            : MakeDecisions(0.03, SafetyAnswer::kOk));
    }
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::move(answers)));
  }

  void AskRelations(const std::string& new_memory,
                    std::vector<std::string> old_memories,
                    RelationsCallback callback) override {
    relation_requests.emplace_back(new_memory, old_memories);
    std::vector<AnswerProbabilities<RelationAnswer>> answers;
    for (const auto& old_memory : old_memories) {
      auto it = relations.find({new_memory, old_memory});
      RelationAnswer answer =
          it != relations.end() ? it->second : RelationAnswer::kDifferent;
      double probability =
          unsure_relations.contains({new_memory, old_memory}) ? 0.5 : 0.95;
      answers.push_back({{answer, probability},
                         {RelationAnswer::kSameTopic, 1.0 - probability}});
    }
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::move(answers)));
  }

  void AskRelevance(const std::string& message,
                    const std::string& previous_message,
                    std::vector<std::string> memories,
                    RelevanceCallback callback) override {
    relevance_requests.push_back({message, previous_message, memories});
    if (on_relevance_asked) {
      std::move(on_relevance_asked).Run();
    }
    if (hold_relevance) {
      return;
    }
    std::optional<std::vector<double>> answer;
    if (!fail_relevance) {
      answer = std::vector<double>();
      for (const auto& memory : memories) {
        auto it = relevance.find(memory);
        answer->push_back(it != relevance.end() ? it->second : 0.05);
      }
    }
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::move(answer)));
  }

  struct RelevanceRequest {
    std::string message;
    std::string previous_message;
    std::vector<std::string> memories;
  };
  // The probability that a memory is relevant, by memory text.
  std::map<std::string, double> relevance;
  bool hold_relevance = false;
  bool fail_relevance = false;
  base::OnceClosure on_relevance_asked;
  std::vector<RelevanceRequest> relevance_requests;

  std::map<std::pair<std::string, std::string>, RelationAnswer> relations;
  // The new memory and the old memories of each relation question.
  std::vector<std::pair<std::string, std::vector<std::string>>>
      relation_requests;
  std::set<std::pair<std::string, std::string>> unsure_relations;
  std::set<std::string> gate_yes;
  std::set<std::string> gate_unsure;
  std::map<std::string, SentenceDecisions> sentence_decisions;
  bool fail_gate = false;
  // The gate never answers.
  bool hold_replies = false;
  base::OnceClosure on_gate_asked;

  std::vector<std::string> gate_requests;
  std::vector<std::string> sentence_requests;
};

// Gives each new text its own direction, so different texts are not close.
// Tests set |vectors| to make texts close.
class FakeEmbedder : public passage_embeddings::Embedder {
 public:
  Job ComputePassagesEmbeddings(
      passage_embeddings::PassagePriority priority,
      std::vector<std::string> passages,
      ComputePassagesEmbeddingsCallback callback) override {
    std::vector<passage_embeddings::Embedding> embeddings;
    for (const auto& passage : passages) {
      embeddings.emplace_back(Vector(passage));
    }
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), passages, std::move(embeddings),
                       next_job_id_,
                       passage_embeddings::ComputeEmbeddingsStatus::kSuccess));
    return Job(weak_ptr_factory_.GetWeakPtr(), next_job_id_++);
  }
  base::WeakPtr<Embedder> GetWeakPtr() override {
    return weak_ptr_factory_.GetWeakPtr();
  }

  std::vector<float> Vector(const std::string& text) {
    if (!vectors.contains(text)) {
      std::vector<float> vector(kSize, 0.0f);
      vector[vectors.size() % kSize] = 1.0f;
      vectors[text] = vector;
    }
    return vectors[text];
  }

  static constexpr size_t kSize = 64;
  std::map<std::string, std::vector<float>> vectors;

 protected:
  void ReprioritizeJobs(passage_embeddings::PassagePriority priority,
                        const std::set<uint64_t>& job_ids) override {}
  bool TryCancel(uint64_t job_id) override { return false; }

 private:
  uint64_t next_job_id_ = 1;
  base::WeakPtrFactory<FakeEmbedder> weak_ptr_factory_{this};
};

// The answers of the local LLM. FakeLlm answers GenerateMemoryText() of a
// MockEngineConsumer with JSON, the same as the model.
struct FakeLlm {
  void Answer(const std::string& system_prompt,
              const std::string& user_message,
              EngineConsumer::GenerationCompletedCallback callback) {
    std::string answer;
    if (user_message.starts_with("Turn date:")) {
      // The lines after the date are "<index>: <sentence>".
      base::ListValue facts;
      std::vector<std::string> lines = base::SplitString(
          user_message, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
      for (size_t i = 1; i < lines.size(); ++i) {
        std::string sentence = lines[i].substr(lines[i].find(": ") + 2);
        if (auto it = rewrites.find(sentence); it != rewrites.end()) {
          facts.Append(base::DictValue()
                           .Set("text", it->second)
                           .Set("sources", base::ListValue().Append(
                                               static_cast<int>(i - 1))));
        }
      }
      answer =
          *base::WriteJson(base::DictValue().Set("facts", std::move(facts)));
    } else if (system_prompt.starts_with("You compare")) {
      ++relation_requests;
      answer = base::StrCat({R"({"relation": ")", relation, R"("})"});
    } else {
      answer = base::StrCat({R"(Sure! {"text": ")", merged, R"("})"});
    }
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback),
                       EngineConsumer::GenerationResultData(
                           mojom::ConversationEntryEvent::NewCompletionEvent(
                               mojom::CompletionEvent::New(answer)),
                           std::nullopt)));
  }

  std::map<std::string, std::string> rewrites;
  std::string relation = "different";
  std::string merged;
  size_t relation_requests = 0;
};

// A data source on a real database. |ready| tells if chat history storage is
// on. While it is off, the methods give empty results, the same as
// AIChatService.
class TestDataSource : public LearnedMemoryDataSource {
 public:
  explicit TestDataSource(base::SequenceBound<AIChatDatabase>& db) : db_(db) {}

  bool IsStorageReady() const override { return ready; }

  void GetStoredConversations(
      base::OnceCallback<void(std::vector<mojom::ConversationPtr>)> callback)
      override {
    if (!ready) {
      RunLater(std::move(callback), std::vector<mojom::ConversationPtr>());
      return;
    }
    db_->AsyncCall(&AIChatDatabase::GetAllConversations)
        .Then(std::move(callback));
  }

  void GetStoredConversationData(
      const std::string& conversation_uuid,
      base::OnceCallback<void(mojom::ConversationArchivePtr)> callback)
      override {
    if (!ready) {
      RunLater(std::move(callback), mojom::ConversationArchivePtr());
      return;
    }
    db_->AsyncCall(&AIChatDatabase::GetConversationData)
        .WithArgs(conversation_uuid)
        .Then(std::move(callback));
  }

  void GetLearnedMemories(
      base::OnceCallback<void(std::vector<LearnedMemory>)> callback) override {
    if (!ready) {
      RunLater(std::move(callback), std::vector<LearnedMemory>());
      return;
    }
    db_->AsyncCall(&AIChatDatabase::GetAllLearnedMemories)
        .Then(std::move(callback));
  }

  void GetLearnedMemoryStamps(
      base::OnceCallback<void(std::vector<LearnedMemoryStamp>)> callback)
      override {
    if (!ready) {
      RunLater(std::move(callback), std::vector<LearnedMemoryStamp>());
      return;
    }
    db_->AsyncCall(&AIChatDatabase::GetLearnedMemoryStamps)
        .Then(std::move(callback));
  }

  void GetLearnedMemoriesByUuid(
      std::vector<std::string> memory_uuids,
      base::OnceCallback<void(std::vector<LearnedMemory>)> callback) override {
    if (!ready) {
      RunLater(std::move(callback), std::vector<LearnedMemory>());
      return;
    }
    db_->AsyncCall(&AIChatDatabase::GetLearnedMemoriesByUuid)
        .WithArgs(std::move(memory_uuids))
        .Then(std::move(callback));
  }

  void GetPermanentLearnedMemories(
      base::OnceCallback<void(std::vector<LearnedMemory>)> callback) override {
    if (!ready) {
      RunLater(std::move(callback), std::vector<LearnedMemory>());
      return;
    }
    db_->AsyncCall(&AIChatDatabase::GetPermanentLearnedMemories)
        .Then(std::move(callback));
  }

  void AddOrUpdateLearnedMemory(
      LearnedMemory memory,
      base::OnceCallback<void(bool)> callback) override {
    if (!ready) {
      RunLater(std::move(callback), false);
      return;
    }
    db_->AsyncCall(&AIChatDatabase::AddOrUpdateLearnedMemory)
        .WithArgs(std::move(memory))
        .Then(std::move(callback));
  }

  void DeleteLearnedMemory(const std::string& memory_uuid,
                           base::OnceCallback<void(bool)> callback) override {
    if (!ready) {
      RunLater(std::move(callback), false);
      return;
    }
    db_->AsyncCall(&AIChatDatabase::DeleteLearnedMemory)
        .WithArgs(memory_uuid)
        .Then(std::move(callback));
  }

  void DeleteAllLearnedMemories(
      base::OnceCallback<void(bool)> callback) override {
    if (!ready) {
      RunLater(std::move(callback), false);
      return;
    }
    db_->AsyncCall(&AIChatDatabase::DeleteAllLearnedMemories)
        .Then(std::move(callback));
  }

  void GetMemoryWatermarks(
      base::OnceCallback<void(std::map<std::string, base::Time>)> callback)
      override {
    if (!ready) {
      RunLater(std::move(callback), std::map<std::string, base::Time>());
      return;
    }
    db_->AsyncCall(&AIChatDatabase::GetAllMemoryWatermarks)
        .Then(std::move(callback));
  }

  void SetMemoryWatermark(const std::string& conversation_uuid,
                          base::Time last_processed_entry_date,
                          base::OnceCallback<void(bool)> callback) override {
    if (!ready) {
      RunLater(std::move(callback), false);
      return;
    }
    db_->AsyncCall(&AIChatDatabase::SetMemoryWatermark)
        .WithArgs(conversation_uuid, last_processed_entry_date)
        .Then(std::move(callback));
  }

  void ImportConversationForEval(
      mojom::ConversationPtr conversation,
      mojom::ConversationTurnPtr first_entry) override {}
  void ImportConversationEntryForEval(
      const std::string& conversation_uuid,
      mojom::ConversationTurnPtr entry) override {}

  bool ready = false;

 private:
  template <typename Result>
  void RunLater(base::OnceCallback<void(Result)> callback, Result result) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::move(result)));
  }

  const raw_ref<base::SequenceBound<AIChatDatabase>> db_;
};

// The index of the learned memories, on FakeEmbedder. Like the real index, it
// follows the database with a delay: it has the memories of its last Sync().
class FakeLearnedMemorySearch : public LearnedMemorySearch {
 public:
  FakeLearnedMemorySearch(base::SequenceBound<AIChatDatabase>& db,
                          FakeEmbedder& embedder)
      : db_(db), embedder_(embedder) {}

  // Puts the memories of the database in the index.
  void Sync() {
    base::test::TestFuture<std::vector<LearnedMemory>> future;
    db_->AsyncCall(&AIChatDatabase::GetAllLearnedMemories)
        .Then(future.GetCallback());
    index_.clear();
    for (const auto& memory : future.Take()) {
      index_[memory.uuid] = embedder_->Vector(memory.text);
    }
  }

  void SetCurrent(bool current) {
    current_ = current;
    if (current_) {
      for (auto& waiter : std::exchange(waiters_, {})) {
        base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
            FROM_HERE, std::move(waiter));
      }
    }
  }

  base::WeakPtr<LearnedMemorySearch> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  // LearnedMemorySearch:
  bool IsLearnedMemoryIndexCurrent() const override { return current_; }
  void WhenLearnedMemoryIndexCurrent(base::OnceClosure callback) override {
    waiters_.push_back(std::move(callback));
    SetCurrent(current_);
  }
  void SearchLearnedMemories(std::vector<std::string> queries,
                             size_t count,
                             SearchCallback callback) override {
    query_requests.push_back(queries);
    std::vector<std::vector<float>> vectors;
    for (const auto& query : queries) {
      vectors.push_back(embedder_->Vector(query));
    }
    Search(vectors, count, std::move(callback));
  }
  void SearchLearnedMemoriesByEmbedding(std::vector<float> embedding,
                                        size_t count,
                                        SearchCallback callback) override {
    Search({std::move(embedding)}, count, std::move(callback));
  }

  std::vector<std::vector<std::string>> query_requests;

 private:
  void Search(const std::vector<std::vector<float>>& queries,
              size_t count,
              SearchCallback callback) {
    std::vector<LearnedMemoryMatch> matches;
    for (const auto& [uuid, vector] : index_) {
      float best = -1.0f;
      for (const auto& query : queries) {
        best = std::max(best, VectorSimilarity(query, vector));
      }
      matches.push_back({uuid, best});
    }
    std::ranges::stable_sort(matches, std::greater<>(),
                             &LearnedMemoryMatch::score);
    matches.resize(std::min(matches.size(), count));
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), std::move(matches)));
  }

  const raw_ref<base::SequenceBound<AIChatDatabase>> db_;
  const raw_ref<FakeEmbedder> embedder_;
  bool current_ = true;
  std::map<std::string, std::vector<float>> index_;
  std::vector<base::OnceClosure> waiters_;
  base::WeakPtrFactory<FakeLearnedMemorySearch> weak_ptr_factory_{this};
};

}  // namespace

class UserMemoryManagerTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_directory_.CreateUniqueTempDir());
    os_crypt_ = os_crypt_async::GetTestOSCryptAsyncForTesting(
        /*is_sync_for_unittests=*/true);
    base::test::TestFuture<scoped_refptr<os_crypt_async::Encryptor>> future;
    os_crypt_->GetInstance(future.GetCallback());
    db_ = base::SequenceBound<AIChatDatabase>(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        temp_directory_.GetPath().AppendASCII("ai_chat"), future.Take());

    prefs_.registry()->RegisterTimePref(prefs::kBraveAIChatLastDreamingTime,
                                        {});
    prefs_.registry()->RegisterBooleanPref(prefs::kBraveAIChatUserMemoryEnabled,
                                           true);
    MakeManager(/*with_llm=*/true);
  }

  void MakeManager(bool with_llm, bool record_trace = false) {
    auto client = std::make_unique<FakeDecisionClient>();
    client_ = client.get();
    // A new engine for each run, the same as AIChatService.
    auto llm_engine_factory = base::BindLambdaForTesting(
        [this, with_llm]() -> std::unique_ptr<EngineConsumer> {
          if (!with_llm) {
            return nullptr;
          }
          auto llm = std::make_unique<testing::NiceMock<MockEngineConsumer>>();
          ON_CALL(*llm, GenerateMemoryText)
              .WillByDefault(testing::Invoke(&llm_, &FakeLlm::Answer));
          return llm;
        });
    manager_.reset();
    DreamingConfig config = UserMemoryManager::GetDreamingConfigFromFeatures();
    config.record_trace = record_trace;
    manager_ = std::make_unique<UserMemoryManager>(
        std::move(client), std::move(llm_engine_factory), &embedder_, &prefs_,
        &data_source_, config);
    manager_->SetLearnedMemorySearch(search_.GetWeakPtr());
  }

  // Chat history storage becomes ready.
  void StorageReady() {
    data_source_.ready = true;
    manager_->OnStorageReady();
  }

  // Chat history storage is turned off.
  void StorageGone() {
    data_source_.ready = false;
    manager_->OnAllConversationsDeleted();
  }

  std::map<std::string, LearnedMemory> MemoriesByText() {
    std::map<std::string, LearnedMemory> result;
    for (auto& memory : GetMemories()) {
      result[memory.text] = std::move(memory);
    }
    return result;
  }

  std::vector<LearnedMemory> GetMemories() {
    base::test::TestFuture<std::vector<LearnedMemory>> future;
    db_.AsyncCall(&AIChatDatabase::GetAllLearnedMemories)
        .Then(future.GetCallback());
    return future.Take();
  }

  // Stores an old learned memory.
  void AddMemory(const std::string& text,
                 LearnedMemoryType type = LearnedMemoryType::kLongTerm) {
    LearnedMemory memory;
    memory.uuid = "old-" + text;
    memory.text = text;
    memory.type = type;
    memory.links = {{"old-chat", "old-entry", 0}};
    base::test::TestFuture<bool> future;
    db_.AsyncCall(&AIChatDatabase::AddOrUpdateLearnedMemory)
        .WithArgs(memory)
        .Then(future.GetCallback());
    ASSERT_TRUE(future.Get());
    search_.Sync();
  }

  // Makes |text| close to |other| (similarity 0.8).
  void MakeClose(const std::string& text, const std::string& other) {
    std::vector<float> vector = embedder_.Vector(other);
    for (auto& value : vector) {
      value *= 0.8f;
    }
    vector[FakeEmbedder::kSize - 1] = 0.6f;
    embedder_.vectors[text] = vector;
  }

  // Asks the gate to keep |turn|, and the decisions to keep |sentence|.
  void KeepSentence(const std::string& turn, const std::string& sentence) {
    client_->gate_yes.insert(turn);
    client_->sentence_decisions[sentence] =
        MakeDecisions(0.95, SafetyAnswer::kOk);
  }

  void TearDown() override {
    manager_.reset();
    // Finish the pending database tasks before the temp dir goes away.
    db_.FlushPostedTasksForTesting();
    db_.Reset();
  }

  // Stores a chat with one user question for each text, from the oldest.
  void AddChat(const std::string& uuid,
               const std::vector<std::string>& texts,
               int hours_ago = 10) {
    base::Time first = base::Time::Now() - base::Hours(hours_ago);
    for (size_t i = 0; i < texts.size(); ++i) {
      auto turn = mojom::ConversationTurn::New(
          uuid + "-entry-" + base::NumberToString(i), std::nullopt,
          mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, texts[i],
          std::nullopt, std::nullopt, std::nullopt, first + base::Minutes(i),
          std::nullopt, std::nullopt, nullptr, false, std::nullopt, nullptr,
          std::vector<std::string>{});
      base::test::TestFuture<bool> future;
      if (i == 0) {
        db_.AsyncCall(&AIChatDatabase::AddConversation)
            .WithArgs(mojom::Conversation::New(
                          uuid, "title", first + base::Minutes(texts.size()),
                          true, std::nullopt, 0, 0, false,
                          std::vector<mojom::AssociatedContentPtr>()),
                      std::vector<std::string>(), std::move(turn))
            .Then(future.GetCallback());
      } else {
        db_.AsyncCall(&AIChatDatabase::AddConversationEntry)
            .WithArgs(uuid, std::move(turn), std::nullopt)
            .Then(future.GetCallback());
      }
      ASSERT_TRUE(future.Get());
    }
  }

  // The index is current when a run starts, so it has the memories of the
  // earlier runs.
  DreamingResult Dream() {
    search_.Sync();
    base::test::TestFuture<DreamingResult> future;
    manager_->LearnFromChats(future.GetCallback());
    return future.Take();
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::ScopedTempDir temp_directory_;
  std::unique_ptr<os_crypt_async::OSCryptAsync> os_crypt_;
  base::SequenceBound<AIChatDatabase> db_;
  TestingPrefServiceSimple prefs_;
  TestDataSource data_source_{db_};
  FakeEmbedder embedder_;
  FakeLearnedMemorySearch search_{db_, embedder_};
  raw_ptr<FakeDecisionClient> client_ = nullptr;
  FakeLlm llm_;
  std::unique_ptr<UserMemoryManager> manager_;
};

TEST_F(UserMemoryManagerTest, UnavailableWithoutStorage) {
  EXPECT_FALSE(manager_->is_storage_ready());
  EXPECT_EQ(Dream().status, DreamingStatus::kUnavailable);
  EXPECT_THAT(client_->gate_requests, IsEmpty());
}

// Chat time (Track B).

class UserMemoryManagerTurnTest : public UserMemoryManagerTest {
 public:
  void SetUp() override {
    UserMemoryManagerTest::SetUp();
    AddMemory("Is vegetarian.");
    AddMemory("Has a beagle named Luna.");
    AddMemory("Writes TypeScript.");
    // The user message is close to the first two memories.
    MakeClose("Suggest a dinner.", "Is vegetarian.");
    StorageReady();
  }

  EngineConsumer::LearnedMemories GetMemoriesForTurn(
      std::vector<std::string> messages) {
    base::test::TestFuture<EngineConsumer::LearnedMemories> future;
    manager_->GetMemoriesForTurn(std::move(messages), future.GetCallback());
    return future.Take();
  }
};

TEST_F(UserMemoryManagerTurnTest, RelevantMemoriesGoWithTheirDate) {
  client_->relevance = {{"Is vegetarian.", 0.9}, {"Writes TypeScript.", 0.1}};

  EngineConsumer::LearnedMemories memories =
      GetMemoriesForTurn({"Suggest a dinner."});

  EXPECT_THAT(memories.permanent, IsEmpty());
  ASSERT_EQ(memories.relevant.size(), 1u);
  // AddMemory() leaves the dates empty, so the text has no date.
  EXPECT_EQ(memories.relevant[0], "Is vegetarian.");
  // The model sees the closest memory first, and the message.
  ASSERT_EQ(client_->relevance_requests.size(), 1u);
  EXPECT_EQ(client_->relevance_requests[0].message, "Suggest a dinner.");
  EXPECT_EQ(client_->relevance_requests[0].memories.front(), "Is vegetarian.");
}

TEST_F(UserMemoryManagerTurnTest, BestMemoriesFirstAndOnlyAboveTheThreshold) {
  client_->relevance = {{"Is vegetarian.", 0.5},
                        {"Has a beagle named Luna.", 0.9},
                        {"Writes TypeScript.", 0.29}};

  EngineConsumer::LearnedMemories memories =
      GetMemoriesForTurn({"Suggest a dinner."});

  EXPECT_THAT(memories.relevant,
              ElementsAre("Has a beagle named Luna.", "Is vegetarian."));
}

TEST_F(UserMemoryManagerTurnTest, PermanentMemoriesAlwaysGo) {
  AddMemory("Allergic to nuts.", LearnedMemoryType::kPermanent);

  EngineConsumer::LearnedMemories memories =
      GetMemoriesForTurn({"Suggest a dinner."});

  EXPECT_THAT(memories.permanent, ElementsAre("Allergic to nuts."));
  // A permanent memory is not asked about.
  for (const auto& memory : client_->relevance_requests[0].memories) {
    EXPECT_NE(memory, "Allergic to nuts.");
  }
}

TEST_F(UserMemoryManagerTurnTest, TwoUserMessagesAreSentToTheModel) {
  GetMemoriesForTurn({"Yes, please.", "Suggest a dinner."});

  ASSERT_EQ(client_->relevance_requests.size(), 1u);
  EXPECT_EQ(client_->relevance_requests[0].message, "Yes, please.");
  EXPECT_EQ(client_->relevance_requests[0].previous_message,
            "Suggest a dinner.");
}

TEST_F(UserMemoryManagerTurnTest, MemoryDeletedAfterTheSyncIsNotSent) {
  // The index still has the memory.
  base::test::TestFuture<bool> deleted;
  db_.AsyncCall(&AIChatDatabase::DeleteLearnedMemory)
      .WithArgs("old-Is vegetarian.")
      .Then(deleted.GetCallback());
  ASSERT_TRUE(deleted.Get());
  client_->relevance = {{"Is vegetarian.", 0.9}};

  EngineConsumer::LearnedMemories memories =
      GetMemoriesForTurn({"Suggest a dinner."});

  EXPECT_THAT(memories.relevant, IsEmpty());
  ASSERT_EQ(client_->relevance_requests.size(), 1u);
  EXPECT_THAT(client_->relevance_requests[0].memories,
              ElementsAre("Has a beagle named Luna.", "Writes TypeScript."));
}

TEST_F(UserMemoryManagerTurnTest, WithoutTheSearchOnlyPermanentMemoriesGo) {
  AddMemory("Allergic to nuts.", LearnedMemoryType::kPermanent);
  manager_->SetLearnedMemorySearch(nullptr);

  EngineConsumer::LearnedMemories memories =
      GetMemoriesForTurn({"Suggest a dinner."});

  EXPECT_THAT(memories.permanent, ElementsAre("Allergic to nuts."));
  EXPECT_THAT(memories.relevant, IsEmpty());
  EXPECT_THAT(client_->relevance_requests, IsEmpty());
}

TEST_F(UserMemoryManagerTurnTest, TimeOutGivesOnlyPermanentMemories) {
  AddMemory("Allergic to nuts.", LearnedMemoryType::kPermanent);
  client_->hold_relevance = true;
  client_->relevance = {{"Is vegetarian.", 0.9}};

  base::test::TestFuture<void> asked;
  client_->on_relevance_asked = asked.GetCallback();
  base::test::TestFuture<EngineConsumer::LearnedMemories> future;
  manager_->GetMemoriesForTurn({"Suggest a dinner."}, future.GetCallback());
  // Wait until the model has the question, then let the time pass.
  ASSERT_TRUE(asked.Wait());
  EXPECT_FALSE(future.IsReady());
  task_environment_.FastForwardBy(TurnMemoryConfig::FromFeatures().timeout);

  EngineConsumer::LearnedMemories memories = future.Take();
  EXPECT_THAT(memories.permanent, ElementsAre("Allergic to nuts."));
  EXPECT_THAT(memories.relevant, IsEmpty());
}

TEST_F(UserMemoryManagerTurnTest, FailedModelGivesNoRelevantMemories) {
  client_->fail_relevance = true;

  EngineConsumer::LearnedMemories memories =
      GetMemoriesForTurn({"Suggest a dinner."});

  EXPECT_TRUE(memories.empty());
}

TEST_F(UserMemoryManagerTurnTest, MemorySettingOffGivesNothing) {
  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);

  EngineConsumer::LearnedMemories memories =
      GetMemoriesForTurn({"Suggest a dinner."});

  EXPECT_TRUE(memories.empty());
  EXPECT_THAT(client_->relevance_requests, IsEmpty());
}

TEST_F(UserMemoryManagerTest, TurnWithoutStorageGivesNothing) {
  base::test::TestFuture<EngineConsumer::LearnedMemories> future;
  manager_->GetMemoriesForTurn({"Suggest a dinner."}, future.GetCallback());
  EXPECT_TRUE(future.Take().empty());
}

TEST(TurnMemoryLookupTest, FormatsTheMemoryWithTheDateOfTheLastMention) {
  LearnedMemory memory;
  memory.text = "Lives in Berlin.";
  base::Time updated;
  ASSERT_TRUE(base::Time::FromUTCString("2026-08-22 10:00:00", &updated));
  memory.updated_date = updated;

  EXPECT_EQ(TurnMemoryLookup::FormatForPrompt(memory),
            "Lives in Berlin. (2026-08-22)");
}

TEST_F(UserMemoryManagerTest, DreamNowLearnsFromChats) {
  AddChat("chat-1", {"I live in Berlin."});
  KeepSentence("I live in Berlin.", "I live in Berlin.");
  StorageReady();

  base::test::TestFuture<DreamingResult> future;
  manager_->DreamNow(future.GetCallback());
  DreamingResult result = future.Take();

  EXPECT_EQ(result.status, DreamingStatus::kCompleted);
  EXPECT_EQ(result.memories_added, 1u);
  base::test::TestFuture<std::vector<LearnedMemory>> memories;
  manager_->GetLearnedMemories(memories.GetCallback());
  ASSERT_EQ(memories.Get().size(), 1u);
  EXPECT_EQ(memories.Get()[0].text, "I live in Berlin.");
}

TEST_F(UserMemoryManagerTest, DreamNowNeedsTheMemorySetting) {
  AddChat("chat-1", {"I live in Berlin."});
  KeepSentence("I live in Berlin.", "I live in Berlin.");
  StorageReady();
  prefs_.SetBoolean(prefs::kBraveAIChatUserMemoryEnabled, false);

  base::test::TestFuture<DreamingResult> future;
  manager_->DreamNow(future.GetCallback());

  EXPECT_EQ(future.Take().status, DreamingStatus::kUnavailable);
  EXPECT_THAT(client_->gate_requests, IsEmpty());
}

TEST_F(UserMemoryManagerTest, DreamNowRunsLongerThanTheDailyRun) {
  AddChat("chat-1", {"I live in Berlin."});
  client_->hold_replies = true;
  StorageReady();

  base::test::TestFuture<void> gate_asked;
  client_->on_gate_asked = gate_asked.GetCallback();
  base::test::TestFuture<DreamingResult> future;
  manager_->DreamNow(future.GetCallback());
  ASSERT_TRUE(gate_asked.Wait());
  // The daily limit passes, and the run continues.
  task_environment_.FastForwardBy(
      UserMemoryManager::GetDreamingConfigFromFeatures().time_limit +
      base::Seconds(1));
  EXPECT_FALSE(future.IsReady());
  task_environment_.FastForwardBy(UserMemoryManager::kDreamNowTimeLimit);

  EXPECT_EQ(future.Take().status, DreamingStatus::kTimedOut);
}

TEST_F(UserMemoryManagerTest, DeleteLearnedMemoryDeletesItForGood) {
  // The user deletes a memory. Nothing remembers it, so a later chat that
  // states the same fact makes a new memory.
  const std::string first_turn = "We just moved to Berlin.";
  AddChat("chat-1", {first_turn}, /*hours_ago=*/30);
  KeepSentence(first_turn, first_turn);
  llm_.rewrites[first_turn] = "Moved to Berlin";
  StorageReady();
  Dream();
  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  const std::string deleted_uuid = memories[0].uuid;

  base::test::TestFuture<bool> deleted;
  manager_->DeleteLearnedMemory(deleted_uuid, deleted.GetCallback());
  EXPECT_TRUE(deleted.Get());
  EXPECT_TRUE(GetMemories().empty());

  const std::string second_turn = "I moved to Berlin last month.";
  AddChat("chat-2", {second_turn}, /*hours_ago=*/1);
  KeepSentence(second_turn, second_turn);
  llm_.rewrites[second_turn] = "Moved to Berlin";
  Dream();

  memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].text, "Moved to Berlin");
  EXPECT_NE(memories[0].uuid, deleted_uuid);
}

TEST_F(UserMemoryManagerTest, DeleteAllLearnedMemoriesKeepsTheWatermarks) {
  AddMemory("Lives in Berlin");
  const std::string turn = "I have a dog.";
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  StorageReady();
  Dream();
  ASSERT_EQ(GetMemories().size(), 2u);

  base::test::TestFuture<bool> deleted;
  manager_->DeleteAllLearnedMemories(deleted.GetCallback());
  EXPECT_TRUE(deleted.Get());
  EXPECT_TRUE(GetMemories().empty());

  // The next run reads only new turns, so the memories do not come back.
  DreamingResult result = Dream();
  EXPECT_EQ(result.status, DreamingStatus::kCompleted);
  EXPECT_EQ(result.turns_read, 0u);
  EXPECT_TRUE(GetMemories().empty());
}

TEST_F(UserMemoryManagerTest, DeleteAllLearnedMemoriesCancelsTheRun) {
  AddMemory("Lives in Berlin");
  AddChat("chat-1", {"I live in Berlin."});
  client_->hold_replies = true;
  StorageReady();
  base::test::TestFuture<void> gate_asked;
  client_->on_gate_asked = gate_asked.GetCallback();
  base::test::TestFuture<DreamingResult> run;
  manager_->LearnFromChats(run.GetCallback());
  ASSERT_TRUE(gate_asked.Wait());

  // The run loaded the memory. It must not write it again.
  base::test::TestFuture<bool> deleted;
  manager_->DeleteAllLearnedMemories(deleted.GetCallback());

  EXPECT_EQ(run.Take().status, DreamingStatus::kCanceled);
  EXPECT_TRUE(deleted.Get());
  EXPECT_TRUE(GetMemories().empty());
  EXPECT_FALSE(manager_->is_dreaming());
  // The daily schedule goes on.
  EXPECT_TRUE(manager_->is_dreaming_scheduled());
}

TEST_F(UserMemoryManagerTest, LearnedMemoriesNeedTheDatabase) {
  base::test::TestFuture<std::vector<LearnedMemory>> memories;
  manager_->GetLearnedMemories(memories.GetCallback());
  EXPECT_TRUE(memories.Get().empty());

  base::test::TestFuture<bool> forgotten;
  manager_->DeleteLearnedMemory("any", forgotten.GetCallback());
  EXPECT_FALSE(forgotten.Get());
}

TEST_F(UserMemoryManagerTest, KeepsCertainFactsOnly) {
  const std::string turn =
      "We just moved to Berlin. My SSN is 123-45-6789. I love hiking. "
      "I have asthma. Work has been crazy lately. Any good trails?";
  AddChat("chat-1", {turn});
  client_->gate_yes.insert(turn);
  client_->sentence_decisions = {
      {"We just moved to Berlin.", MakeDecisions(0.95, SafetyAnswer::kOk)},
      {"I love hiking.", MakeDecisions(0.92, SafetyAnswer::kOk,
                                       LearnedMemoryCategory::kPreference)},
      {"I have asthma.", MakeDecisions(0.90, SafetyAnswer::kSensitive)},
      {"Work has been crazy lately.", MakeDecisions(0.58, SafetyAnswer::kOk)},
  };
  StorageReady();

  DreamingResult result = Dream();

  EXPECT_EQ(result.status, DreamingStatus::kCompleted);
  EXPECT_EQ(result.turns_read, 1u);
  EXPECT_EQ(result.turns_kept, 1u);
  EXPECT_EQ(result.memories_added, 2u);
  std::map<std::string, LearnedMemory> memories = MemoriesByText();
  ASSERT_EQ(memories.size(), 2u);
  const LearnedMemory& berlin = memories["We just moved to Berlin."];
  EXPECT_EQ(berlin.category, LearnedMemoryCategory::kPersonalFact);
  EXPECT_EQ(berlin.type, LearnedMemoryType::kLongTerm);
  // The sentence index is the place in the turn, with the SSN sentence counted.
  EXPECT_THAT(berlin.links,
              ElementsAre(MemorySourceLink{"chat-1", "chat-1-entry-0", 0}));
  const LearnedMemory& hiking = memories["I love hiking."];
  EXPECT_EQ(hiking.category, LearnedMemoryCategory::kPreference);
  EXPECT_THAT(hiking.links,
              ElementsAre(MemorySourceLink{"chat-1", "chat-1-entry-0", 2}));
}

TEST_F(UserMemoryManagerTest, DeniedPatternNeverReachesTheDecisionModel) {
  const std::string turn = "My SSN is 123-45-6789. I live in Berlin.";
  AddChat("chat-1", {turn});
  client_->gate_yes.insert(turn);
  StorageReady();

  Dream();

  EXPECT_THAT(client_->sentence_requests, ElementsAre("I live in Berlin."));
}

TEST_F(UserMemoryManagerTest, GateBelowTheThresholdSkipsTheTurn) {
  AddChat("chat-1", {"Work has been crazy lately."});
  client_->gate_unsure.insert("Work has been crazy lately.");
  StorageReady();

  DreamingResult result = Dream();

  EXPECT_EQ(result.status, DreamingStatus::kCompleted);
  EXPECT_EQ(result.turns_read, 1u);
  EXPECT_EQ(result.turns_kept, 0u);
  EXPECT_THAT(client_->sentence_requests, IsEmpty());
}

TEST_F(UserMemoryManagerTest, TemporaryStateIsShortTerm) {
  const std::string turn = "I am in Tokyo this week.";
  AddChat("chat-1", {turn});
  client_->gate_yes.insert(turn);
  client_->sentence_decisions = {
      {turn, MakeDecisions(0.9, SafetyAnswer::kOk,
                           LearnedMemoryCategory::kPersonalFact, 0.95)}};
  StorageReady();

  Dream();

  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].type, LearnedMemoryType::kShortTerm);
}

TEST_F(UserMemoryManagerTest, LabelsThatAreNotCertainUseNeutralValues) {
  const std::string turn = "I run every morning.";
  AddChat("chat-1", {turn});
  client_->gate_yes.insert(turn);
  SentenceDecisions decisions = MakeDecisions(0.9, SafetyAnswer::kOk);
  decisions.category = {{LearnedMemoryCategory::kPreference, 0.5},
                        {LearnedMemoryCategory::kTopic, 0.4}};
  decisions.temporary = {{true, 0.55}, {false, 0.45}};
  client_->sentence_decisions = {{turn, decisions}};
  StorageReady();

  Dream();

  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].category, LearnedMemoryCategory::kPersonalFact);
  EXPECT_EQ(memories[0].type, LearnedMemoryType::kLongTerm);
}

TEST_F(UserMemoryManagerTest, OnlyTurnsAfterTheWatermarkAreRead) {
  AddChat("chat-1", {"old question", "new question"});
  // The watermark is the date of the first turn.
  base::test::TestFuture<bool> future;
  db_.AsyncCall(&AIChatDatabase::SetMemoryWatermark)
      .WithArgs(std::string("chat-1"), base::Time::Now() - base::Hours(10))
      .Then(future.GetCallback());
  ASSERT_TRUE(future.Get());
  StorageReady();

  DreamingResult result = Dream();

  EXPECT_EQ(result.turns_read, 1u);
  EXPECT_THAT(client_->gate_requests, ElementsAre("new question"));

  // The run moved the watermark, so the next run reads no turn.
  EXPECT_EQ(Dream().turns_read, 0u);
}

TEST_F(UserMemoryManagerTest, TurnsOfAllChatsGoByDate) {
  // The chat ids sort the other way round: "chat-1" is newer than "chat-2".
  AddChat("chat-1", {"newer question"}, /*hours_ago=*/1);
  AddChat("chat-2", {"older question"}, /*hours_ago=*/5);
  StorageReady();

  Dream();

  EXPECT_THAT(client_->gate_requests,
              ElementsAre("older question", "newer question"));
}

TEST_F(UserMemoryManagerTest, GateFailureFailsTheRun) {
  AddChat("chat-1", {"I live in Berlin."});
  client_->fail_gate = true;
  StorageReady();

  DreamingResult result = Dream();

  EXPECT_EQ(result.status, DreamingStatus::kFailed);
  EXPECT_TRUE(GetMemories().empty());
  EXPECT_FALSE(manager_->is_dreaming());
}

TEST_F(UserMemoryManagerTest, OnlyOneRunAtATime) {
  AddChat("chat-1", {"I live in Berlin."});
  client_->hold_replies = true;
  StorageReady();

  base::test::TestFuture<DreamingResult> first;
  manager_->LearnFromChats(first.GetCallback());
  EXPECT_TRUE(manager_->is_dreaming());

  EXPECT_EQ(Dream().status, DreamingStatus::kBusy);
  EXPECT_FALSE(first.IsReady());

  StorageGone();
  EXPECT_EQ(first.Take().status, DreamingStatus::kCanceled);
}

TEST_F(UserMemoryManagerTest, RunStopsAtTheTimeLimit) {
  AddChat("chat-1", {"I live in Berlin."});
  client_->hold_replies = true;
  StorageReady();

  base::test::TestFuture<void> gate_asked;
  client_->on_gate_asked = gate_asked.GetCallback();
  base::test::TestFuture<DreamingResult> future;
  manager_->LearnFromChats(future.GetCallback());
  // Wait until the run asks the gate, then let the time pass.
  ASSERT_TRUE(gate_asked.Wait());
  EXPECT_FALSE(future.IsReady());
  task_environment_.FastForwardBy(
      UserMemoryManager::GetDreamingConfigFromFeatures().time_limit);

  EXPECT_EQ(future.Take().status, DreamingStatus::kTimedOut);
  EXPECT_FALSE(manager_->is_dreaming());
}

TEST_F(UserMemoryManagerTest, StorageGoneStopsTheRun) {
  AddChat("chat-1", {"I live in Berlin."});
  client_->hold_replies = true;
  StorageReady();

  base::test::TestFuture<void> gate_asked;
  client_->on_gate_asked = gate_asked.GetCallback();
  base::test::TestFuture<DreamingResult> future;
  manager_->LearnFromChats(future.GetCallback());
  ASSERT_TRUE(gate_asked.Wait());
  StorageGone();

  EXPECT_EQ(future.Take().status, DreamingStatus::kCanceled);
  EXPECT_FALSE(manager_->is_storage_ready());
  // A new run needs storage.
  EXPECT_EQ(Dream().status, DreamingStatus::kUnavailable);
}

TEST_F(UserMemoryManagerTest, RewriteIsStored) {
  const std::string turn = "We just moved to Berlin.";
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  llm_.rewrites[turn] = "Moved to Berlin";
  StorageReady();

  Dream();

  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].text, "Moved to Berlin");
}

TEST_F(UserMemoryManagerTest, RewriteThatIsTooLongUsesTheUserSentence) {
  const std::string turn = "We just moved to Berlin.";
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  llm_.rewrites[turn] = std::string(kMaxMemoryTextLength + 1, 'a');
  StorageReady();

  Dream();

  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].text, turn);
}

TEST_F(UserMemoryManagerTest, RewriteWithANewNameIsStored) {
  // The user reviews the memories, so a new name is only a flag in the trace.
  MakeManager(/*with_llm=*/true, /*record_trace=*/true);
  const std::string turn = "We just moved to Berlin.";
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  llm_.rewrites[turn] = "Moved to Berlin from Paris";
  StorageReady();

  DreamingResult result = Dream();

  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].text, "Moved to Berlin from Paris");
  for (const auto& step : result.trace) {
    if (*step.GetDict().FindString("step") == "rewrite_parsed") {
      const auto& fact = step.GetDict().FindList("facts")->front().GetDict();
      EXPECT_EQ(fact.FindBool("no_new_details"), false);
    }
  }
}

TEST_F(UserMemoryManagerTest, ReplaceKeepsTheOldText) {
  const std::string turn = "We just moved to Berlin.";
  AddMemory("Lives in San Francisco");
  MakeClose(turn, "Lives in San Francisco");
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  client_->relations[{turn, "Lives in San Francisco"}] =
      RelationAnswer::kReplace;
  StorageReady();

  DreamingResult result = Dream();

  EXPECT_EQ(result.memories_updated, 1u);
  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].text, turn);
  ASSERT_TRUE(memories[0].previous);
  EXPECT_EQ(memories[0].previous->text, "Lives in San Francisco");
  EXPECT_THAT(memories[0].links,
              ElementsAre(MemorySourceLink{"chat-1", "chat-1-entry-0", 0}));
}

TEST_F(UserMemoryManagerTest, ShortTermNeverReplaces) {
  const std::string turn = "I am in Tokyo this week.";
  AddMemory("Lives in Berlin");
  MakeClose(turn, "Lives in Berlin");
  AddChat("chat-1", {turn});
  client_->gate_yes.insert(turn);
  client_->sentence_decisions[turn] = MakeDecisions(
      0.95, SafetyAnswer::kOk, LearnedMemoryCategory::kPersonalFact, 0.95);
  client_->relations[{turn, "Lives in Berlin"}] = RelationAnswer::kReplace;
  StorageReady();

  Dream();

  std::map<std::string, LearnedMemory> memories = MemoriesByText();
  EXPECT_EQ(memories.size(), 2u);
  EXPECT_FALSE(memories["Lives in Berlin"].previous);
}

TEST_F(UserMemoryManagerTest, NotCertainRelationAsksTheLlm) {
  const std::string turn = "I am in Tokyo.";
  AddMemory("Lives in Berlin");
  MakeClose(turn, "Lives in Berlin");
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  client_->unsure_relations.insert({turn, "Lives in Berlin"});
  llm_.relation = "different";
  StorageReady();

  Dream();

  EXPECT_EQ(llm_.relation_requests, 1u);
  EXPECT_EQ(GetMemories().size(), 2u);
}

TEST_F(UserMemoryManagerTest, LlmFallbackNeverReplacesOrMerges) {
  // The decision model is not certain, and the LLM says "merge". A merge needs
  // a certain decision model, so both memories stay.
  const std::string turn = "We adopted a second dog.";
  AddMemory("Has a dog");
  MakeClose(turn, "Has a dog");
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  client_->relations[{turn, "Has a dog"}] = RelationAnswer::kMerge;
  client_->unsure_relations.insert({turn, "Has a dog"});
  llm_.relation = "merge";
  llm_.merged = "Has two dogs";
  StorageReady();

  Dream();

  EXPECT_EQ(llm_.relation_requests, 1u);
  std::map<std::string, LearnedMemory> memories = MemoriesByText();
  EXPECT_EQ(memories.size(), 2u);
  EXPECT_FALSE(memories["Has a dog"].previous);
}

TEST_F(UserMemoryManagerTest, NotCertainRelationWithoutLlmIsNotAdded) {
  MakeManager(/*with_llm=*/false);
  const std::string turn = "I am in Tokyo.";
  AddMemory("Lives in Berlin");
  MakeClose(turn, "Lives in Berlin");
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  client_->unsure_relations.insert({turn, "Lives in Berlin"});
  StorageReady();

  Dream();

  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].text, "Lives in Berlin");
}

TEST_F(UserMemoryManagerTest, MergeWritesOneMemory) {
  const std::string turn = "We adopted a second dog.";
  AddMemory("Has a dog");
  MakeClose(turn, "Has a dog");
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  client_->relations[{turn, "Has a dog"}] = RelationAnswer::kMerge;
  llm_.merged = "Has two dogs";
  // Close to both inputs.
  MakeClose("Has two dogs", "Has a dog");
  embedder_.vectors["Has two dogs"] = embedder_.vectors[turn];
  StorageReady();

  Dream();

  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].text, "Has two dogs");
  EXPECT_EQ(memories[0].previous->text, "Has a dog");
  EXPECT_EQ(memories[0].links.size(), 2u);
}

TEST_F(UserMemoryManagerTest, MemoriesOfTheSameRunAreNeighbors) {
  // The index does not have the first memory during the run.
  const std::string first = "I live in San Francisco.";
  const std::string second = "We just moved to Berlin.";
  MakeClose(second, first);
  AddChat("chat-1", {first, second});
  KeepSentence(first, first);
  KeepSentence(second, second);
  client_->relations[{second, first}] = RelationAnswer::kReplace;
  StorageReady();

  Dream();

  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].text, second);
  ASSERT_TRUE(memories[0].previous);
  EXPECT_EQ(memories[0].previous->text, first);
}

TEST_F(UserMemoryManagerTest, ChangedMemoryIsComparedByItsNewText) {
  const std::string old_text = "Lives in San Francisco";
  const std::string moved = "We just moved to Berlin.";
  const std::string visit = "I love the Bay Area.";
  AddMemory(old_text);
  MakeClose(moved, old_text);
  // Close to the old text (0.6), not to the new text (0.48).
  std::vector<float> visit_vector = embedder_.Vector("Far from everything");
  for (size_t i = 0; i < FakeEmbedder::kSize; ++i) {
    visit_vector[i] =
        0.6f * embedder_.Vector(old_text)[i] + 0.8f * visit_vector[i];
  }
  embedder_.vectors[visit] = visit_vector;
  AddChat("chat-1", {moved, visit});
  KeepSentence(moved, moved);
  KeepSentence(visit, visit);
  client_->relations[{moved, old_text}] = RelationAnswer::kReplace;
  StorageReady();

  Dream();

  // The index still has the embedding of the old text, but the run compares
  // with the new text.
  ASSERT_EQ(client_->relation_requests.size(), 1u);
  EXPECT_EQ(client_->relation_requests[0].first, moved);
  std::map<std::string, LearnedMemory> memories = MemoriesByText();
  EXPECT_EQ(memories.size(), 2u);
  EXPECT_TRUE(memories.contains(moved));
  EXPECT_TRUE(memories.contains(visit));
}

TEST_F(UserMemoryManagerTest, RunWaitsUntilTheIndexIsCurrent) {
  AddChat("chat-1", {"I live in Berlin."});
  StorageReady();
  search_.SetCurrent(false);

  base::test::TestFuture<DreamingResult> future;
  manager_->LearnFromChats(future.GetCallback());
  // The wait does not count for the time limit of the run.
  task_environment_.FastForwardBy(
      UserMemoryManager::GetDreamingConfigFromFeatures().time_limit +
      base::Seconds(10));
  EXPECT_THAT(client_->gate_requests, IsEmpty());
  EXPECT_FALSE(future.IsReady());

  search_.SetCurrent(true);
  EXPECT_EQ(future.Take().status, DreamingStatus::kCompleted);
  EXPECT_THAT(client_->gate_requests, ElementsAre("I live in Berlin."));
}

TEST_F(UserMemoryManagerTest, RunFailsWhenTheIndexStaysBehind) {
  AddChat("chat-1", {"I live in Berlin."});
  StorageReady();
  search_.SetCurrent(false);

  base::test::TestFuture<DreamingResult> future;
  manager_->LearnFromChats(future.GetCallback());
  task_environment_.FastForwardBy(DreamingConfig().index_wait_limit);

  EXPECT_EQ(future.Take().status, DreamingStatus::kFailed);
  EXPECT_THAT(client_->gate_requests, IsEmpty());
  // The manager tries again later.
  EXPECT_TRUE(manager_->is_dreaming_scheduled());
}

TEST_F(UserMemoryManagerTest, DreamingNeedsTheSearch) {
  AddChat("chat-1", {"I live in Berlin."});
  manager_->SetLearnedMemorySearch(nullptr);
  StorageReady();

  EXPECT_EQ(Dream().status, DreamingStatus::kUnavailable);
  // The timer checks again later, and runs when the search is there.
  task_environment_.FastForwardBy(UserMemoryManager::kFirstRunDelay);
  EXPECT_THAT(client_->gate_requests, IsEmpty());
  EXPECT_TRUE(manager_->is_dreaming_scheduled());

  manager_->SetLearnedMemorySearch(search_.GetWeakPtr());
  task_environment_.FastForwardBy(UserMemoryManager::kRetryDelay);
  EXPECT_THAT(client_->gate_requests, ElementsAre("I live in Berlin."));
}

TEST_F(UserMemoryManagerTest, TimerRunsDreamingEachDay) {
  AddChat("chat-1", {"I live in Berlin."});
  StorageReady();
  EXPECT_TRUE(manager_->is_dreaming_scheduled());
  EXPECT_TRUE(client_->gate_requests.empty());

  // The first run starts after the first run delay, and saves its time.
  task_environment_.FastForwardBy(UserMemoryManager::kFirstRunDelay);
  task_environment_.RunUntilIdle();
  EXPECT_THAT(client_->gate_requests, ElementsAre("I live in Berlin."));
  EXPECT_EQ(prefs_.GetTime(prefs::kBraveAIChatLastDreamingTime),
            base::Time::Now());
  EXPECT_TRUE(manager_->is_dreaming_scheduled());

  // The next run is one day later, and it reads only the new turn.
  AddChat("chat-2", {"I have a dog."});
  task_environment_.FastForwardBy(UserMemoryManager::kDreamingInterval);
  task_environment_.RunUntilIdle();
  EXPECT_THAT(client_->gate_requests,
              ElementsAre("I live in Berlin.", "I have a dog."));
}

TEST_F(UserMemoryManagerTest, AllChatsDeletedKeepsTheScheduleWhileStorageIsOn) {
  StorageReady();
  ASSERT_TRUE(manager_->is_dreaming_scheduled());

  // The user deleted all chats. Storage is on, so the daily schedule goes on.
  manager_->OnAllConversationsDeleted();

  EXPECT_TRUE(manager_->is_storage_ready());
  EXPECT_TRUE(manager_->is_dreaming_scheduled());
}

TEST_F(UserMemoryManagerTest, AllChatsDeletedCancelsTheRun) {
  AddChat("chat-1", {"I live in Berlin."});
  client_->hold_replies = true;
  StorageReady();
  base::test::TestFuture<void> gate_asked;
  client_->on_gate_asked = gate_asked.GetCallback();
  base::test::TestFuture<DreamingResult> future;
  manager_->LearnFromChats(future.GetCallback());
  ASSERT_TRUE(gate_asked.Wait());

  // Storage stays on. The chats and the memories are gone, so the run stops,
  // and the next run is scheduled.
  manager_->OnAllConversationsDeleted();

  EXPECT_EQ(future.Take().status, DreamingStatus::kCanceled);
  EXPECT_FALSE(manager_->is_dreaming());
  EXPECT_TRUE(manager_->is_dreaming_scheduled());
}

TEST_F(UserMemoryManagerTest, NoTimerWithoutStorage) {
  StorageReady();
  StorageGone();
  EXPECT_FALSE(manager_->is_dreaming_scheduled());
}

TEST_F(UserMemoryManagerTest, TraceRecordsEachStep) {
  MakeManager(/*with_llm=*/true, /*record_trace=*/true);
  const std::string turn = "We just moved to Berlin.";
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  llm_.rewrites[turn] = "Moved to Berlin";
  embedder_.vectors["Moved to Berlin"] = embedder_.Vector(turn);
  StorageReady();

  DreamingResult result = Dream();

  std::vector<std::string> steps;
  for (const auto& step : result.trace) {
    steps.push_back(*step.GetDict().FindString("step"));
  }
  EXPECT_THAT(steps,
              ElementsAre("index_current", "loaded", "turn", "gate", "split",
                          "sentence_decisions", "llm_call", "rewrite_parsed",
                          "embed", "rewrite_checked", "fact", "neighbors",
                          "store", "watermark", "done"));
  // Without the trace, the result has no steps.
  MakeManager(/*with_llm=*/true);
  StorageReady();
  EXPECT_TRUE(Dream().trace.empty());
}

TEST(LearnedMemoryEvalTest, ParseChatSet) {
  const base::Time now = base::Time::Now();
  auto chats = LearnedMemoryEval::ParseChatSet(
      R"({"name": "x", "expect": {}, "chats": [
          {"title": "Gyms", "days_ago": 2, "turns": [
            {"user": "We moved to Berlin.", "assistant": "Welcome!"},
            {"user": "Any gyms?"}]}]})",
      now);
  ASSERT_TRUE(chats);
  ASSERT_EQ(chats->size(), 1u);
  const EvalChat& chat = (*chats)[0];
  EXPECT_EQ(chat.conversation->uuid, "eval-0");
  EXPECT_EQ(chat.conversation->title, "Gyms");
  ASSERT_EQ(chat.entries.size(), 4u);
  EXPECT_EQ(chat.entries[0]->uuid, "eval-0-0-user");
  EXPECT_EQ(chat.entries[0]->text, "We moved to Berlin.");
  EXPECT_EQ(chat.entries[0]->created_time, now - base::Days(2));
  EXPECT_EQ(chat.entries[1]->character_type, mojom::CharacterType::ASSISTANT);
  EXPECT_EQ(chat.entries[3]->text, "OK.");
  EXPECT_LT(chat.entries[2]->created_time, chat.entries[3]->created_time);

  EXPECT_FALSE(LearnedMemoryEval::ParseChatSet(R"({"chats": [{}]})", now));
  EXPECT_FALSE(LearnedMemoryEval::ParseChatSet("not json", now));
}

TEST_F(UserMemoryManagerTest, GetLearnableText) {
  auto make = [](mojom::CharacterType character, mojom::ActionType action,
                 const std::string& text) {
    return mojom::ConversationTurn::New(
        "uuid", std::nullopt, character, action, text, std::nullopt,
        std::nullopt, std::nullopt, base::Time::Now(), std::nullopt,
        std::nullopt, nullptr, false, std::nullopt, nullptr,
        std::vector<std::string>{});
  };

  EXPECT_EQ(DreamingRun::GetLearnableText(*make(mojom::CharacterType::HUMAN,
                                                mojom::ActionType::QUERY,
                                                "  I live in Berlin.  ")),
            "I live in Berlin.");
  // Not typed by the user, or not a normal question.
  EXPECT_FALSE(DreamingRun::GetLearnableText(*make(
      mojom::CharacterType::ASSISTANT, mojom::ActionType::RESPONSE, "Hi")));
  EXPECT_FALSE(DreamingRun::GetLearnableText(*make(
      mojom::CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_PAGE, "Hi")));
  EXPECT_FALSE(DreamingRun::GetLearnableText(
      *make(mojom::CharacterType::HUMAN, mojom::ActionType::QUERY, "  ")));
  // A long paste.
  EXPECT_FALSE(DreamingRun::GetLearnableText(
      *make(mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
            std::string(DreamingRun::kMaxTurnLength + 1, 'a'))));
  EXPECT_TRUE(DreamingRun::GetLearnableText(
      *make(mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
            std::string(DreamingRun::kMaxTurnLength, 'a'))));

  // An edited turn uses the last edit.
  auto edited = make(mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
                     "I live in Paris.");
  edited->edits.emplace();
  edited->edits->push_back(make(mojom::CharacterType::HUMAN,
                                mojom::ActionType::QUERY, "I live in Rome."));
  edited->edits->push_back(make(mojom::CharacterType::HUMAN,
                                mojom::ActionType::QUERY, "I live in Berlin."));
  EXPECT_EQ(DreamingRun::GetLearnableText(*edited), "I live in Berlin.");
}

}  // namespace ai_chat
