// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/dreaming_run.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/dreaming_text_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

namespace {

// Embeddings are unit length, so the dot product is the cosine similarity.
float Similarity(const std::vector<float>& a, const std::vector<float>& b) {
  if (a.size() != b.size()) {
    // A different model version: the vectors cannot be compared.
    return 0.0f;
  }
  float sum = 0.0f;
  for (size_t i = 0; i < a.size(); ++i) {
    sum += a[i] * b[i];
  }
  return sum;
}

void AddLinks(std::vector<MemorySourceLink>& links,
              const std::vector<MemorySourceLink>& more) {
  for (const auto& link : more) {
    if (!std::ranges::contains(links, link)) {
      links.push_back(link);
    }
  }
}

// A short-term memory never replaces or merges, and Dreaming does not change
// a memory that the user wants to keep forever.
bool CanChangeText(LearnedMemoryType new_type, LearnedMemoryType old_type) {
  return new_type != LearnedMemoryType::kShortTerm &&
         old_type == LearnedMemoryType::kLongTerm;
}

template <typename Answer>
double ProbabilityOf(const AnswerProbabilities<Answer>& probabilities,
                     Answer answer) {
  auto it = probabilities.find(answer);
  return it == probabilities.end() ? 0.0 : it->second;
}

}  // namespace

const char* DreamingStatusToString(DreamingStatus status) {
  switch (status) {
    case DreamingStatus::kCompleted:
      return "completed";
    case DreamingStatus::kTimedOut:
      return "timed_out";
    case DreamingStatus::kFailed:
      return "failed";
    case DreamingStatus::kCanceled:
      return "canceled";
    case DreamingStatus::kBusy:
      return "busy";
    case DreamingStatus::kUnavailable:
      return "unavailable";
  }
}

DreamingResult::DreamingResult() = default;
DreamingResult::DreamingResult(DreamingStatus status) : status(status) {}
DreamingResult::DreamingResult(DreamingResult&&) = default;
DreamingResult& DreamingResult::operator=(DreamingResult&&) = default;
DreamingResult::~DreamingResult() = default;

DreamingRun::Fact::Fact() = default;
DreamingRun::Fact::Fact(Fact&&) = default;
DreamingRun::Fact& DreamingRun::Fact::operator=(Fact&&) = default;
DreamingRun::Fact::~Fact() = default;

DreamingRun::DreamingRun(base::SequenceBound<AIChatDatabase>& db,
                         MemoryDecisionClient& decision_client,
                         EngineConsumer* llm_engine,
                         passage_embeddings::Embedder& embedder,
                         DreamingConfig config,
                         DoneCallback done)
    : db_(db),
      decision_client_(decision_client),
      llm_engine_(llm_engine),
      embedder_(embedder),
      config_(config),
      done_(std::move(done)) {}

DreamingRun::~DreamingRun() = default;

// static
std::optional<std::string> DreamingRun::GetLearnableText(
    const mojom::ConversationTurn& turn) {
  if (!turn.uuid || turn.character_type != mojom::CharacterType::HUMAN ||
      turn.action_type != mojom::ActionType::QUERY) {
    return std::nullopt;
  }
  const std::string* text = &turn.text;
  if (turn.edits && !turn.edits->empty()) {
    text = &turn.edits->back()->text;
  }
  std::string trimmed(base::TrimWhitespaceASCII(*text, base::TRIM_ALL));
  if (trimmed.empty() || base::UTF8ToUTF16(trimmed).size() > kMaxTurnLength) {
    return std::nullopt;
  }
  return trimmed;
}

void DreamingRun::Start() {
  time_limit_timer_.Start(
      FROM_HERE, config_.time_limit,
      base::BindOnce(&DreamingRun::Finish, base::Unretained(this),
                     DreamingStatus::kTimedOut));
  db_->AsyncCall(&AIChatDatabase::GetAllMemoryWatermarks)
      .Then(base::BindOnce(&DreamingRun::OnWatermarks,
                           weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::Cancel() {
  Finish(DreamingStatus::kCanceled);
}

void DreamingRun::OnWatermarks(std::map<std::string, base::Time> watermarks) {
  watermarks_ = std::move(watermarks);
  db_->AsyncCall(&AIChatDatabase::GetAllLearnedMemories)
      .Then(base::BindOnce(&DreamingRun::OnMemories,
                           weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnMemories(std::vector<LearnedMemory> memories) {
  memories_ = std::move(memories);
  db_->AsyncCall(&AIChatDatabase::GetAllMemoryTombstones)
      .Then(base::BindOnce(&DreamingRun::OnTombstones,
                           weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnTombstones(std::vector<MemoryTombstone> tombstones) {
  tombstones_ = std::move(tombstones);
  db_->AsyncCall(&AIChatDatabase::GetAllConversations)
      .Then(base::BindOnce(&DreamingRun::OnConversations,
                           weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnConversations(
    std::vector<mojom::ConversationPtr> conversations) {
  // Read each chat, and use only the turns after its watermark. Do not filter
  // by updated_time: GetAllConversations() can give the date of any turn of
  // the chat, not the date of the last turn. The database does not store
  // temporary chats, so Dreaming never reads them.
  for (const auto& conversation : conversations) {
    conversations_to_read_.push_back(conversation->uuid);
  }
  ReadNextConversation();
}

void DreamingRun::ReadNextConversation() {
  if (conversations_to_read_.empty()) {
    // The database gives the chats in the order of their ids, which is random.
    // A replace or a merge needs the old fact first, so go by date.
    std::stable_sort(
        turns_.begin(), turns_.end(),
        [](const UserTurn& a, const UserTurn& b) { return a.date < b.date; });
    ProcessNextTurn();
    return;
  }
  std::string uuid = std::move(conversations_to_read_.front());
  conversations_to_read_.pop_front();
  base::Time watermark;
  if (auto it = watermarks_.find(uuid); it != watermarks_.end()) {
    watermark = it->second;
  }
  db_->AsyncCall(&AIChatDatabase::GetConversationData)
      .WithArgs(uuid)
      .Then(base::BindOnce(&DreamingRun::OnConversationData,
                           weak_ptr_factory_.GetWeakPtr(), uuid, watermark));
}

void DreamingRun::OnConversationData(std::string conversation_uuid,
                                     base::Time watermark,
                                     mojom::ConversationArchivePtr archive) {
  if (archive) {
    for (const auto& entry : archive->entries) {
      if (entry->created_time <= watermark) {
        continue;
      }
      std::optional<std::string> text = GetLearnableText(*entry);
      if (!text) {
        continue;
      }
      turns_.push_back({conversation_uuid, *entry->uuid, std::move(*text),
                        entry->created_time});
    }
  }
  ReadNextConversation();
}

void DreamingRun::ProcessNextTurn() {
  if (turns_.empty()) {
    Finish(DreamingStatus::kCompleted);
    return;
  }
  ++result_.turns_read;
  candidates_.clear();
  facts_.clear();
  next_fact_ = 0;
  decision_client_->AskGate(
      turn().text,
      base::BindOnce(&DreamingRun::OnGate, weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnGate(std::optional<AnswerProbabilities<bool>> gate) {
  if (!gate) {
    Finish(DreamingStatus::kFailed);
    return;
  }
  // The gate removes only turns that are clearly not about the user, so it
  // uses its own, lower threshold.
  const bool keep = ProbabilityOf(*gate, true) >= config_.gate_threshold;
  if (!keep) {
    FinishTurn();
    return;
  }
  ++result_.turns_kept;

  // The index of a sentence is its place in the turn, before the removal of
  // sentences with a denied pattern. The links use this index.
  std::vector<std::string> all_sentences = SplitIntoSentences(turn().text);
  std::vector<size_t> sentence_indexes;
  std::vector<std::string> sentences;
  for (size_t i = 0; i < all_sentences.size(); ++i) {
    // A sentence with a denied pattern never reaches a model.
    if (HasDeniedPattern(all_sentences[i])) {
      continue;
    }
    sentence_indexes.push_back(i);
    sentences.push_back(std::move(all_sentences[i]));
  }
  if (sentences.empty()) {
    FinishTurn();
    return;
  }
  std::vector<std::string> request = sentences;
  decision_client_->AskSentenceDecisions(
      std::move(request),
      base::BindOnce(&DreamingRun::OnSentenceDecisions,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(sentence_indexes), std::move(sentences)));
}

void DreamingRun::OnSentenceDecisions(
    std::vector<size_t> sentence_indexes,
    std::vector<std::string> sentences,
    std::optional<std::vector<SentenceDecisions>> decisions) {
  if (!decisions || decisions->size() != sentences.size()) {
    Finish(DreamingStatus::kFailed);
    return;
  }
  for (size_t i = 0; i < sentences.size(); ++i) {
    const SentenceDecisions& decision = (*decisions)[i];
    // The sentence must be a fact about the user, and no unsafe answer may
    // reach its limit.
    const struct {
      SafetyAnswer answer;
      double limit;
    } kLimits[] = {{SafetyAnswer::kSensitive, config_.max_sensitive},
                   {SafetyAnswer::kInstruction, config_.max_instruction},
                   {SafetyAnswer::kShortLived, config_.max_short_lived}};
    bool keep = ProbabilityOf(decision.fact, true) >= config_.fact_threshold;
    for (const auto& [answer, limit] : kLimits) {
      if (ProbabilityOf(decision.safety, answer) >= limit) {
        keep = false;
      }
    }
    if (!keep) {
      continue;
    }
    Candidate candidate;
    candidate.text = std::move(sentences[i]);
    // Neutral labels when not certain: a personal fact, and long-term.
    candidate.category = Certain(decision.category)
                             .value_or(LearnedMemoryCategory::kPersonalFact);
    candidate.type = Certain(decision.temporary).value_or(false)
                         ? LearnedMemoryType::kShortTerm
                         : LearnedMemoryType::kLongTerm;
    candidate.link = {turn().conversation_uuid, turn().entry_uuid,
                      static_cast<uint32_t>(sentence_indexes[i])};
    candidates_.push_back(std::move(candidate));
  }
  if (candidates_.empty()) {
    FinishTurn();
    return;
  }
  // Only the kept sentences reach the LLM.
  if (!CanWrite()) {
    OnRewrite(std::nullopt);
    return;
  }
  ++writing_requests_;
  std::vector<std::string> texts;
  for (const auto& candidate : candidates_) {
    texts.push_back(candidate.text);
  }
  AskLlm(
      BuildRewriteRequest(
          std::move(texts), turn().date,
          [this](std::string& input) { llm_engine_->SanitizeInput(input); }),
      base::BindOnce(&DreamingRun::OnRewrite, weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnRewrite(std::optional<std::string> answer) {
  std::optional<std::vector<RewrittenFact>> rewritten;
  if (answer) {
    rewritten = ParseRewriteAnswer(*answer, candidates_.size());
  }
  // The guards are mechanical: ParseRewriteAnswer() drops a fact with no text
  // or no valid source, and a fact that is too long is dropped here. The user
  // reviews the memories.
  std::vector<bool> covered(candidates_.size(), false);
  for (auto& rewritten_fact :
       rewritten.value_or(std::vector<RewrittenFact>())) {
    const bool too_long =
        base::UTF8ToUTF16(rewritten_fact.text).size() > kMaxMemoryTextLength;
    if (too_long) {
      continue;
    }
    // The labels come from the decision model, for the first source.
    const Candidate& first = candidates_[rewritten_fact.sources[0]];
    Fact fact;
    fact.text = std::move(rewritten_fact.text);
    fact.category = first.category;
    fact.type = first.type;
    for (size_t index : rewritten_fact.sources) {
      fact.links.push_back(candidates_[index].link);
      covered[index] = true;
    }
    facts_.push_back(std::move(fact));
  }
  // The user's sentence is the fallback for a sentence that no fact covers.
  for (size_t i = 0; i < candidates_.size(); ++i) {
    if (covered[i]) {
      continue;
    }
    Fact fact;
    fact.text = candidates_[i].text;
    fact.category = candidates_[i].category;
    fact.type = candidates_[i].type;
    fact.links = {candidates_[i].link};
    facts_.push_back(std::move(fact));
  }
  std::vector<std::string> passages;
  for (const auto& fact : facts_) {
    passages.push_back(fact.text);
  }
  Embed(std::move(passages), base::BindOnce(&DreamingRun::OnFactEmbeddings,
                                            weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnFactEmbeddings(std::vector<std::vector<float>> vectors) {
  CHECK_EQ(vectors.size(), facts_.size());
  for (size_t i = 0; i < facts_.size(); ++i) {
    facts_[i].vector = std::move(vectors[i]);
  }
  next_fact_ = 0;
  ProcessNextFact();
}

void DreamingRun::FinishTurn() {
  db_->AsyncCall(&AIChatDatabase::SetMemoryWatermark)
      .WithArgs(turn().conversation_uuid, turn().date)
      .Then(base::BindOnce(&DreamingRun::OnWatermarkSet,
                           weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnWatermarkSet(bool success) {
  if (!success) {
    Finish(DreamingStatus::kFailed);
    return;
  }
  turns_.pop_front();
  ProcessNextTurn();
}

void DreamingRun::ProcessNextFact() {
  if (next_fact_ >= facts_.size()) {
    FinishTurn();
    return;
  }
  for (const auto& tombstone : tombstones_) {
    const float similarity = Similarity(fact().vector, tombstone.vector);
    if (similarity >= config_.tombstone_similarity) {
      // The user deleted a memory like this one.
      NextFact();
      return;
    }
  }
  std::vector<std::pair<float, size_t>> scored;
  for (size_t i = 0; i < memories_.size(); ++i) {
    float score = Similarity(fact().vector, memories_[i].vector);
    if (score >= config_.min_neighbor_similarity) {
      scored.emplace_back(score, i);
    }
  }
  std::ranges::sort(scored, std::ranges::greater());
  if (scored.size() > config_.max_neighbors) {
    scored.resize(config_.max_neighbors);
  }
  if (scored.empty()) {
    AddNewMemory();
    return;
  }
  neighbors_.clear();
  std::vector<std::string> old_texts;
  for (const auto& [score, index] : scored) {
    neighbors_.push_back(index);
    old_texts.push_back(memories_[index].text);
  }
  decision_client_->AskRelations(
      fact().text, std::move(old_texts),
      base::BindOnce(&DreamingRun::OnRelations,
                     weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnRelations(
    std::optional<std::vector<AnswerProbabilities<RelationAnswer>>> relations) {
  if (!relations || relations->size() != neighbors_.size()) {
    Finish(DreamingStatus::kFailed);
    return;
  }
  relations_ = std::move(*relations);
  next_relation_ = 0;
  EvaluateNextRelation();
}

void DreamingRun::EvaluateNextRelation() {
  // The closest old memory first. The first relation that is not "different"
  // decides.
  if (next_relation_ >= neighbors_.size()) {
    AddNewMemory();
    return;
  }
  if (std::optional<RelationAnswer> relation =
          Certain(relations_[next_relation_])) {
    ApplyRelation(*relation, /*certain=*/true);
    return;
  }
  // Not certain: ask the LLM. Without the LLM, do not add the fact. It can
  // come back in a later chat.
  if (!llm_engine_ || relation_requests_ >= config_.max_relation_requests) {
    NextFact();
    return;
  }
  ++relation_requests_;
  AskLlm(BuildRelationRequest(
             fact().text, neighbor().text,
             [this](std::string& input) { llm_engine_->SanitizeInput(input); }),
         base::BindOnce(&DreamingRun::OnLlmRelation,
                        weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnLlmRelation(std::optional<std::string> answer) {
  std::optional<RelationAnswer> relation =
      answer ? ParseRelationAnswer(*answer) : std::nullopt;
  if (!relation) {
    NextFact();
    return;
  }
  ApplyRelation(*relation, /*certain=*/false);
}

void DreamingRun::ApplyRelation(RelationAnswer relation, bool certain) {
  const LearnedMemory& old = neighbor();
  // The text of an old memory changes only when the rules allow it, and the
  // decision model was certain. A wrong replace or merge loses information.
  const bool can_change = CanChangeText(fact().type, old.type) && certain;
  if (relation == RelationAnswer::kSame) {
    LearnedMemory updated = old;
    updated.updated_date = std::max(old.updated_date, turn().date);
    AddLinks(updated.links, fact().links);
    Store(std::move(updated), /*is_new=*/false);
    return;
  }
  if (relation == RelationAnswer::kReplace && can_change) {
    LearnedMemory updated = old;
    updated.previous = PreviousMemoryText{old.text, old.vector, old.links};
    updated.text = fact().text;
    updated.vector = fact().vector;
    updated.links = fact().links;
    updated.category = fact().category;
    updated.updated_date = turn().date;
    Store(std::move(updated), /*is_new=*/false);
    return;
  }
  if (relation == RelationAnswer::kMerge && can_change) {
    // Without the LLM, both memories stay.
    if (!CanWrite()) {
      AddNewMemory();
      return;
    }
    ++writing_requests_;
    AskLlm(
        BuildMergeRequest(
            old.text, fact().text,
            [this](std::string& input) { llm_engine_->SanitizeInput(input); }),
        base::BindOnce(&DreamingRun::OnMerged, weak_ptr_factory_.GetWeakPtr()));
    return;
  }
  // Different, same topic (topics come later), or a change that the rules do
  // not allow: compare with the next old memory.
  ++next_relation_;
  EvaluateNextRelation();
}

void DreamingRun::OnMerged(std::optional<std::string> answer) {
  std::optional<std::string> merged =
      answer ? ParseMergeAnswer(*answer) : std::nullopt;
  // Mechanical guards only, the same as for a rewrite. ParseMergeAnswer()
  // drops an empty text.
  const bool too_long =
      merged && base::UTF8ToUTF16(*merged).size() > kMaxMemoryTextLength;
  if (!merged || too_long) {
    // Both memories stay.
    AddNewMemory();
    return;
  }
  std::string text = *merged;
  Embed({std::move(*merged)},
        base::BindOnce(&DreamingRun::OnMergedEmbedding,
                       weak_ptr_factory_.GetWeakPtr(), std::move(text)));
}

void DreamingRun::OnMergedEmbedding(std::string merged,
                                    std::vector<std::vector<float>> vectors) {
  const LearnedMemory& old = neighbor();
  LearnedMemory updated = old;
  updated.previous = PreviousMemoryText{old.text, old.vector, old.links};
  updated.text = std::move(merged);
  updated.vector = std::move(vectors[0]);
  AddLinks(updated.links, fact().links);
  updated.updated_date = turn().date;
  Store(std::move(updated), /*is_new=*/false);
}

void DreamingRun::AddNewMemory() {
  LearnedMemory memory;
  memory.uuid = base::Uuid::GenerateRandomV4().AsLowercaseString();
  memory.text = fact().text;
  memory.vector = fact().vector;
  memory.category = fact().category;
  memory.type = fact().type;
  memory.created_date = turn().date;
  memory.updated_date = turn().date;
  memory.last_used_date = turn().date;
  memory.links = fact().links;
  Store(std::move(memory), /*is_new=*/true);
}

void DreamingRun::Store(LearnedMemory memory, bool is_new) {
  LearnedMemory copy = memory;
  db_->AsyncCall(&AIChatDatabase::AddOrUpdateLearnedMemory)
      .WithArgs(std::move(copy))
      .Then(base::BindOnce(&DreamingRun::OnStored,
                           weak_ptr_factory_.GetWeakPtr(), std::move(memory),
                           is_new));
}

void DreamingRun::OnStored(LearnedMemory memory, bool is_new, bool success) {
  if (!success) {
    Finish(DreamingStatus::kFailed);
    return;
  }
  if (is_new) {
    ++result_.memories_added;
    memories_.push_back(std::move(memory));
  } else {
    ++result_.memories_updated;
    auto it = std::ranges::find(memories_, memory.uuid, &LearnedMemory::uuid);
    CHECK(it != memories_.end());
    *it = std::move(memory);
  }
  NextFact();
}

void DreamingRun::NextFact() {
  ++next_fact_;
  ProcessNextFact();
}

bool DreamingRun::CanWrite() const {
  return llm_engine_ && writing_requests_ < config_.max_writing_requests;
}

void DreamingRun::AskLlm(MemoryLlmRequest request, LlmCallback callback) {
  llm_engine_->GenerateMemoryText(
      request.system_prompt, request.user_message,
      base::BindOnce(&DreamingRun::OnLlmAnswer, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void DreamingRun::OnLlmAnswer(LlmCallback callback,
                              EngineConsumer::GenerationResult result) {
  // A failed LLM request does not stop the run: the step uses its fallback.
  std::optional<std::string> answer;
  if (result.has_value() && result->event &&
      result->event->is_completion_event()) {
    answer = result->event->get_completion_event()->completion;
  }
  std::move(callback).Run(std::move(answer));
}

void DreamingRun::Embed(std::vector<std::string> passages,
                        EmbeddingsCallback callback) {
  embed_job_ = embedder_->ComputePassagesEmbeddings(
      passage_embeddings::PassagePriority::kPassive, std::move(passages),
      base::BindOnce(&DreamingRun::OnEmbedded, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void DreamingRun::OnEmbedded(
    EmbeddingsCallback callback,
    std::vector<std::string> passages,
    std::vector<passage_embeddings::Embedding> embeddings,
    uint64_t job_id,
    passage_embeddings::ComputeEmbeddingsStatus status) {
  const bool success =
      status == passage_embeddings::ComputeEmbeddingsStatus::kSuccess &&
      embeddings.size() == passages.size();
  if (!success) {
    Finish(DreamingStatus::kFailed);
    return;
  }
  std::vector<std::vector<float>> vectors;
  for (const auto& embedding : embeddings) {
    vectors.push_back(embedding.GetData());
  }
  std::move(callback).Run(std::move(vectors));
}

void DreamingRun::Finish(DreamingStatus status) {
  if (!done_) {
    return;
  }
  time_limit_timer_.Stop();
  weak_ptr_factory_.InvalidateWeakPtrs();
  embed_job_.reset();
  result_.status = status;
  // The owner can delete this object in |done_|, so this must be the last
  // use of the members.
  std::move(done_).Run(std::move(result_));
}

}  // namespace ai_chat
