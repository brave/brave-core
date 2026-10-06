// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/user_memory_manager.h"

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
#include "brave/components/ai_chat/core/browser/learned_memory_eval.h"
#include "brave/components/ai_chat/core/browser/memory_decision_client.h"
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

  std::map<std::pair<std::string, std::string>, RelationAnswer> relations;
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
        config);
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
    memory.vector = embedder_.Vector(text);
    memory.type = type;
    memory.links = {{"old-chat", "old-entry", 0}};
    base::test::TestFuture<bool> future;
    db_.AsyncCall(&AIChatDatabase::AddOrUpdateLearnedMemory)
        .WithArgs(memory)
        .Then(future.GetCallback());
    ASSERT_TRUE(future.Get());
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

  DreamingResult Dream() {
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
  FakeEmbedder embedder_;
  raw_ptr<FakeDecisionClient> client_ = nullptr;
  FakeLlm llm_;
  std::unique_ptr<UserMemoryManager> manager_;
};

TEST_F(UserMemoryManagerTest, UnavailableWithoutDatabase) {
  EXPECT_FALSE(manager_->is_database_available());
  EXPECT_EQ(Dream().status, DreamingStatus::kUnavailable);
  EXPECT_THAT(client_->gate_requests, IsEmpty());
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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

  Dream();

  EXPECT_THAT(client_->sentence_requests, ElementsAre("I live in Berlin."));
}

TEST_F(UserMemoryManagerTest, GateBelowTheThresholdSkipsTheTurn) {
  AddChat("chat-1", {"Work has been crazy lately."});
  client_->gate_unsure.insert("Work has been crazy lately.");
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

  Dream();

  EXPECT_THAT(client_->gate_requests,
              ElementsAre("older question", "newer question"));
}

TEST_F(UserMemoryManagerTest, GateFailureFailsTheRun) {
  AddChat("chat-1", {"I live in Berlin."});
  client_->fail_gate = true;
  manager_->OnDatabaseAvailable(&db_);

  DreamingResult result = Dream();

  EXPECT_EQ(result.status, DreamingStatus::kFailed);
  EXPECT_TRUE(GetMemories().empty());
  EXPECT_FALSE(manager_->is_dreaming());
}

TEST_F(UserMemoryManagerTest, OnlyOneRunAtATime) {
  AddChat("chat-1", {"I live in Berlin."});
  client_->hold_replies = true;
  manager_->OnDatabaseAvailable(&db_);

  base::test::TestFuture<DreamingResult> first;
  manager_->LearnFromChats(first.GetCallback());
  EXPECT_TRUE(manager_->is_dreaming());

  EXPECT_EQ(Dream().status, DreamingStatus::kBusy);
  EXPECT_FALSE(first.IsReady());

  manager_->OnDatabaseUnavailable();
  EXPECT_EQ(first.Take().status, DreamingStatus::kCanceled);
}

TEST_F(UserMemoryManagerTest, RunStopsAtTheTimeLimit) {
  AddChat("chat-1", {"I live in Berlin."});
  client_->hold_replies = true;
  manager_->OnDatabaseAvailable(&db_);

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

TEST_F(UserMemoryManagerTest, DatabaseGoneStopsTheRun) {
  AddChat("chat-1", {"I live in Berlin."});
  client_->hold_replies = true;
  manager_->OnDatabaseAvailable(&db_);

  base::test::TestFuture<void> gate_asked;
  client_->on_gate_asked = gate_asked.GetCallback();
  base::test::TestFuture<DreamingResult> future;
  manager_->LearnFromChats(future.GetCallback());
  ASSERT_TRUE(gate_asked.Wait());
  manager_->OnDatabaseUnavailable();

  EXPECT_EQ(future.Take().status, DreamingStatus::kCanceled);
  EXPECT_FALSE(manager_->is_database_available());
  // A new run needs a database.
  EXPECT_EQ(Dream().status, DreamingStatus::kUnavailable);
}

TEST_F(UserMemoryManagerTest, RewriteIsStored) {
  const std::string turn = "We just moved to Berlin.";
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  llm_.rewrites[turn] = "Moved to Berlin";
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

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
  manager_->OnDatabaseAvailable(&db_);

  Dream();

  auto memories = GetMemories();
  ASSERT_EQ(memories.size(), 1u);
  EXPECT_EQ(memories[0].text, "Has two dogs");
  EXPECT_EQ(memories[0].previous->text, "Has a dog");
  EXPECT_EQ(memories[0].links.size(), 2u);
}

TEST_F(UserMemoryManagerTest, TimerRunsDreamingEachDay) {
  AddChat("chat-1", {"I live in Berlin."});
  manager_->OnDatabaseAvailable(&db_);
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

TEST_F(UserMemoryManagerTest, NoTimerWithoutDatabase) {
  manager_->OnDatabaseAvailable(&db_);
  manager_->OnDatabaseUnavailable();
  EXPECT_FALSE(manager_->is_dreaming_scheduled());
}

TEST_F(UserMemoryManagerTest, TraceRecordsEachStep) {
  MakeManager(/*with_llm=*/true, /*record_trace=*/true);
  const std::string turn = "We just moved to Berlin.";
  AddChat("chat-1", {turn});
  KeepSentence(turn, turn);
  llm_.rewrites[turn] = "Moved to Berlin";
  embedder_.vectors["Moved to Berlin"] = embedder_.Vector(turn);
  manager_->OnDatabaseAvailable(&db_);

  DreamingResult result = Dream();

  std::vector<std::string> steps;
  for (const auto& step : result.trace) {
    steps.push_back(*step.GetDict().FindString("step"));
  }
  EXPECT_THAT(
      steps,
      ElementsAre("loaded", "turn", "gate", "split", "sentence_decisions",
                  "llm_call", "rewrite_parsed", "embed", "rewrite_checked",
                  "fact", "neighbors", "store", "watermark", "done"));
  // Without the trace, the result has no steps.
  MakeManager(/*with_llm=*/true);
  manager_->OnDatabaseAvailable(&db_);
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
