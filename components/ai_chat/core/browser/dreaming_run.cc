// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/dreaming_run.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/dreaming_text_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

namespace {

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

const char* Name(bool answer) {
  return answer ? "yes" : "no";
}

const char* Name(SafetyAnswer answer) {
  switch (answer) {
    case SafetyAnswer::kOk:
      return "ok";
    case SafetyAnswer::kSensitive:
      return "sensitive";
    case SafetyAnswer::kInstruction:
      return "instruction";
    case SafetyAnswer::kShortLived:
      return "short_lived";
    case SafetyAnswer::kNotAboutUser:
      return "not_about_user";
  }
}

const char* Name(LearnedMemoryCategory category) {
  return LearnedMemoryCategoryToString(category);
}

const char* Name(LearnedMemoryType type) {
  return LearnedMemoryTypeToString(type);
}

const char* Name(RelationAnswer answer) {
  switch (answer) {
    case RelationAnswer::kDifferent:
      return "different";
    case RelationAnswer::kSame:
      return "same";
    case RelationAnswer::kReplace:
      return "replace";
    case RelationAnswer::kMerge:
      return "merge";
    case RelationAnswer::kSameTopic:
      return "same_topic";
  }
}

template <typename Answer>
base::DictValue ToDict(const AnswerProbabilities<Answer>& probabilities) {
  base::DictValue dict;
  for (const auto& [answer, probability] : probabilities) {
    dict.Set(Name(answer), probability);
  }
  return dict;
}

template <typename Answer>
double ProbabilityOf(const AnswerProbabilities<Answer>& probabilities,
                     Answer answer) {
  auto it = probabilities.find(answer);
  return it == probabilities.end() ? 0.0 : it->second;
}

template <typename Answer>
base::Value CertainToValue(const std::optional<Answer>& answer) {
  return answer ? base::Value(Name(*answer)) : base::Value("not_certain");
}

base::ListValue LinksToList(const std::vector<MemorySourceLink>& links) {
  base::ListValue list;
  for (const auto& link : links) {
    list.Append(base::DictValue()
                    .Set("conversation", link.conversation_uuid)
                    .Set("entry", link.entry_uuid)
                    .Set("sentence", static_cast<int>(link.sentence_index)));
  }
  return list;
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
    case DreamingStatus::kReviewPending:
      return "review_pending";
  }
}

const char* LearnedMemoryCategoryToString(LearnedMemoryCategory category) {
  switch (category) {
    case LearnedMemoryCategory::kPreference:
      return "preference";
    case LearnedMemoryCategory::kPersonalFact:
      return "personal_fact";
    case LearnedMemoryCategory::kTopic:
      return "topic";
  }
}

const char* LearnedMemoryTypeToString(LearnedMemoryType type) {
  switch (type) {
    case LearnedMemoryType::kPermanent:
      return "permanent";
    case LearnedMemoryType::kLongTerm:
      return "long_term";
    case LearnedMemoryType::kShortTerm:
      return "short_term";
  }
}

DreamingResult::DreamingResult() = default;
DreamingResult::DreamingResult(DreamingStatus status) : status(status) {}
DreamingResult::DreamingResult(DreamingResult&&) = default;
DreamingResult& DreamingResult::operator=(DreamingResult&&) = default;
DreamingResult::~DreamingResult() = default;

DreamingProposal::DreamingProposal() = default;
DreamingProposal::DreamingProposal(const DreamingProposal&) = default;
DreamingProposal& DreamingProposal::operator=(const DreamingProposal&) =
    default;
DreamingProposal::DreamingProposal(DreamingProposal&&) = default;
DreamingProposal& DreamingProposal::operator=(DreamingProposal&&) = default;
DreamingProposal::~DreamingProposal() = default;

DreamingReview::DreamingReview() = default;
DreamingReview::DreamingReview(DreamingReview&&) = default;
DreamingReview& DreamingReview::operator=(DreamingReview&&) = default;
DreamingReview::~DreamingReview() = default;

DreamingRun::Fact::Fact() = default;
DreamingRun::Fact::Fact(Fact&&) = default;
DreamingRun::Fact& DreamingRun::Fact::operator=(Fact&&) = default;
DreamingRun::Fact::~Fact() = default;

DreamingRun::DreamingRun(LearnedMemoryDataSource& data_source,
                         base::WeakPtr<LearnedMemorySearch> search,
                         MemoryDecisionClient& decision_client,
                         EngineConsumer* llm_engine,
                         passage_embeddings::Embedder& embedder,
                         DreamingConfig config,
                         DoneCallback done)
    : data_source_(data_source),
      search_(std::move(search)),
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
  run_start_ = base::TimeTicks::Now();
  if (!search_) {
    Finish(DreamingStatus::kFailed);
    return;
  }
  // During a rebuild of the index, the neighbor search would miss old
  // memories, and the run would write duplicates.
  index_wait_timer_.Start(
      FROM_HERE, config_.index_wait_limit,
      base::BindOnce(&DreamingRun::Finish, base::Unretained(this),
                     DreamingStatus::kFailed));
  search_->WhenLearnedMemoryIndexCurrent(base::BindOnce(
      &DreamingRun::OnIndexCurrent, weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnIndexCurrent() {
  index_wait_timer_.Stop();
  Trace("index_current", base::DictValue());
  time_limit_timer_.Start(
      FROM_HERE, config_.time_limit,
      base::BindOnce(&DreamingRun::Finish, base::Unretained(this),
                     DreamingStatus::kTimedOut));
  data_source_->GetMemoryWatermarks(base::BindOnce(
      &DreamingRun::OnWatermarks, weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::Cancel() {
  Finish(DreamingStatus::kCanceled);
}

void DreamingRun::OnWatermarks(std::map<std::string, base::Time> watermarks) {
  watermarks_ = std::move(watermarks);
  data_source_->GetLearnedMemories(
      base::BindOnce(&DreamingRun::OnMemories, weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnMemories(std::vector<LearnedMemory> memories) {
  memories_ = std::move(memories);
  if (config_.review_changes) {
    stored_memories_ = memories_;
  }
  if (tracing()) {
    base::ListValue texts;
    for (const auto& memory : memories_) {
      texts.Append(memory.text);
    }
    Trace("loaded",
          base::DictValue()
              .Set("memories", std::move(texts))
              .Set("watermarks", static_cast<int>(watermarks_.size())));
  }
  data_source_->GetStoredConversations(base::BindOnce(
      &DreamingRun::OnConversations, weak_ptr_factory_.GetWeakPtr()));
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
  data_source_->GetStoredConversationData(
      uuid, base::BindOnce(&DreamingRun::OnConversationData,
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

void DreamingRun::Trace(std::string_view step, base::DictValue data) {
  if (!tracing()) {
    return;
  }
  data.Set("step", step);
  data.Set("t_ms", static_cast<int>(
                       (base::TimeTicks::Now() - run_start_).InMilliseconds()));
  if (!turns_.empty()) {
    data.Set("entry", turn().entry_uuid);
  }
  result_.trace.Append(std::move(data));
}

void DreamingRun::ProcessNextTurn() {
  if (turns_.empty()) {
    Finish(DreamingStatus::kCompleted);
    return;
  }
  ++result_.turns_read;
  if (config_.review_changes) {
    turn_start_memories_ = memories_;
  }
  candidates_.clear();
  facts_.clear();
  next_fact_ = 0;
  Trace("turn", base::DictValue()
                    .Set("conversation", turn().conversation_uuid)
                    .Set("date", base::TimeFormatAsIso8601(turn().date))
                    .Set("text", turn().text));
  StartCall();
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
  Trace("gate", base::DictValue()
                    .Set("latency_ms", static_cast<int>(CallMs()))
                    .Set("probabilities", ToDict(*gate))
                    .Set("threshold", config_.gate_threshold)
                    .Set("result", keep ? "keep" : "skip"));
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
  base::ListValue split;
  for (size_t i = 0; i < all_sentences.size(); ++i) {
    // A sentence with a denied pattern never reaches a model.
    const bool denied = HasDeniedPattern(all_sentences[i]);
    if (tracing()) {
      split.Append(base::DictValue()
                       .Set("index", static_cast<int>(i))
                       .Set("text", all_sentences[i])
                       .Set("denylist", denied));
    }
    if (denied) {
      continue;
    }
    sentence_indexes.push_back(i);
    sentences.push_back(std::move(all_sentences[i]));
  }
  Trace("split", base::DictValue().Set("sentences", std::move(split)));
  if (sentences.empty()) {
    FinishTurn();
    return;
  }
  std::vector<std::string> request = sentences;
  StartCall();
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
  base::ListValue traced;
  for (size_t i = 0; i < sentences.size(); ++i) {
    const SentenceDecisions& decision = (*decisions)[i];
    // The sentence must be a fact about the user, and no unsafe answer may
    // reach its limit. The reason is for the trace.
    const double fact_yes = ProbabilityOf(decision.fact, true);
    const struct {
      SafetyAnswer answer;
      double limit;
    } kLimits[] = {{SafetyAnswer::kSensitive, config_.max_sensitive},
                   {SafetyAnswer::kInstruction, config_.max_instruction},
                   {SafetyAnswer::kShortLived, config_.max_short_lived}};
    std::string drop_reason;
    if (fact_yes < config_.fact_threshold) {
      drop_reason = base::StringPrintf("no fact (%.2f < %.2f)", fact_yes,
                                       config_.fact_threshold);
    }
    for (const auto& [answer, limit] : kLimits) {
      const double probability = ProbabilityOf(decision.safety, answer);
      if (drop_reason.empty() && probability >= limit) {
        drop_reason = base::StringPrintf("%s (%.2f >= %.2f)", Name(answer),
                                         probability, limit);
      }
    }
    const bool keep = drop_reason.empty();
    if (tracing()) {
      const std::string result =
          keep ? "keep" : base::StrCat({"drop: ", drop_reason});
      traced.Append(base::DictValue()
                        .Set("index", static_cast<int>(sentence_indexes[i]))
                        .Set("text", sentences[i])
                        .Set("fact", ToDict(decision.fact))
                        .Set("safety", ToDict(decision.safety))
                        .Set("category", ToDict(decision.category))
                        .Set("temporary", ToDict(decision.temporary))
                        .Set("category_answer",
                             CertainToValue(Certain(decision.category)))
                        .Set("temporary_answer",
                             CertainToValue(Certain(decision.temporary)))
                        .Set("result", result));
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
  Trace("sentence_decisions", base::DictValue()
                                  .Set("latency_ms", static_cast<int>(CallMs()))
                                  .Set("sentences", std::move(traced)));
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
      "rewrite",
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
  // reviews the memories. IsFaithfulRewrite() only writes a flag in the trace.
  std::vector<bool> covered(candidates_.size(), false);
  base::ListValue traced;
  for (auto& rewritten_fact :
       rewritten.value_or(std::vector<RewrittenFact>())) {
    std::vector<std::string> sources;
    base::ListValue source_indexes;
    for (size_t index : rewritten_fact.sources) {
      sources.push_back(candidates_[index].text);
      source_indexes.Append(static_cast<int>(index));
    }
    const bool too_long =
        base::UTF8ToUTF16(rewritten_fact.text).size() > kMaxMemoryTextLength;
    if (tracing()) {
      traced.Append(
          base::DictValue()
              .Set("text", rewritten_fact.text)
              .Set("sources", std::move(source_indexes))
              .Set("no_new_details", IsFaithfulRewrite(rewritten_fact.text,
                                                       sources, {turn().date}))
              .Set("too_long", too_long));
    }
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
  Trace("rewrite_parsed", base::DictValue()
                              .Set("used_llm", answer.has_value())
                              .Set("parsed", rewritten.has_value())
                              .Set("facts", std::move(traced)));
  std::vector<std::string> passages;
  for (const auto& fact : facts_) {
    passages.push_back(fact.text);
  }
  Embed("facts", std::move(passages),
        base::BindOnce(&DreamingRun::OnFactEmbeddings,
                       weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnFactEmbeddings(std::vector<std::vector<float>> vectors) {
  CHECK_EQ(vectors.size(), facts_.size());
  for (size_t i = 0; i < facts_.size(); ++i) {
    facts_[i].vector = std::move(vectors[i]);
  }
  if (tracing()) {
    base::ListValue facts;
    for (const auto& fact : facts_) {
      facts.Append(base::DictValue()
                       .Set("text", fact.text)
                       .Set("category", Name(fact.category))
                       .Set("type", Name(fact.type)));
    }
    Trace("rewrite_checked", base::DictValue().Set("facts", std::move(facts)));
  }
  next_fact_ = 0;
  ProcessNextFact();
}

void DreamingRun::FinishTurn() {
  Trace("watermark",
        base::DictValue().Set("date", base::TimeFormatAsIso8601(turn().date)));
  if (config_.review_changes) {
    base::Time& watermark = review_watermarks_[turn().conversation_uuid];
    watermark = std::max(watermark, turn().date);
    OnWatermarkSet(true);
    return;
  }
  data_source_->SetMemoryWatermark(
      turn().conversation_uuid, turn().date,
      base::BindOnce(&DreamingRun::OnWatermarkSet,
                     weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnWatermarkSet(bool success) {
  if (!success) {
    Finish(DreamingStatus::kFailed);
    return;
  }
  turns_.pop_front();
  turn_start_memories_.reset();
  ProcessNextTurn();
}

void DreamingRun::ProcessNextFact() {
  if (next_fact_ >= facts_.size()) {
    FinishTurn();
    return;
  }
  Trace("fact", base::DictValue()
                    .Set("text", fact().text)
                    .Set("category", Name(fact().category))
                    .Set("type", Name(fact().type))
                    .Set("links", LinksToList(fact().links)));
  if (!search_) {
    Finish(DreamingStatus::kFailed);
    return;
  }
  // The index can give the memories that this run changed, with the score of
  // their old text. They are left out below, so ask for more.
  StartCall();
  search_->SearchLearnedMemoriesByEmbedding(
      fact().vector, config_.max_neighbors + run_embeddings_.size(),
      base::BindOnce(&DreamingRun::OnNeighborMatches,
                     weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnNeighborMatches(std::vector<LearnedMemoryMatch> matches) {
  auto find_memory = [this](const std::string& uuid) -> std::optional<size_t> {
    auto it = std::ranges::find(memories_, uuid, &LearnedMemory::uuid);
    if (it == memories_.end()) {
      return std::nullopt;
    }
    return static_cast<size_t>(it - memories_.begin());
  };
  std::vector<std::pair<float, size_t>> scored;
  for (const auto& match : matches) {
    // A memory that this run changed gets its score from its new text below.
    // A memory that is not loaded was written after the start of the run.
    std::optional<size_t> index = find_memory(match.uuid);
    if (index && !run_embeddings_.contains(match.uuid)) {
      scored.emplace_back(match.score, *index);
    }
  }
  for (const auto& [uuid, embedding] : run_embeddings_) {
    if (std::optional<size_t> index = find_memory(uuid)) {
      scored.emplace_back(VectorSimilarity(fact().vector, embedding), *index);
    }
  }
  std::ranges::sort(scored, std::ranges::greater());
  const float best = scored.empty() ? 0.0f : scored.front().first;
  std::erase_if(scored, [this](const std::pair<float, size_t>& item) {
    return item.first < config_.min_neighbor_similarity;
  });
  if (scored.size() > config_.max_neighbors) {
    scored.resize(config_.max_neighbors);
  }
  if (tracing()) {
    base::ListValue neighbors;
    for (const auto& [score, index] : scored) {
      neighbors.Append(base::DictValue()
                           .Set("text", memories_[index].text)
                           .Set("similarity", score));
    }
    Trace("neighbors", base::DictValue()
                           .Set("latency_ms", static_cast<int>(CallMs()))
                           .Set("memories", static_cast<int>(memories_.size()))
                           .Set("best_similarity", best)
                           .Set("floor", config_.min_neighbor_similarity)
                           .Set("neighbors", std::move(neighbors)));
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
  StartCall();
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
  if (tracing()) {
    base::ListValue pairs;
    for (size_t i = 0; i < relations_.size(); ++i) {
      pairs.Append(base::DictValue()
                       .Set("old", memories_[neighbors_[i]].text)
                       .Set("probabilities", ToDict(relations_[i]))
                       .Set("answer", CertainToValue(Certain(relations_[i]))));
    }
    Trace("relations", base::DictValue()
                           .Set("latency_ms", static_cast<int>(CallMs()))
                           .Set("new", fact().text)
                           .Set("pairs", std::move(pairs)));
  }
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
    Trace("fact_dropped",
          base::DictValue()
              .Set("reason", "relation not certain, and no LLM left")
              .Set("old", neighbor().text));
    NextFact();
    return;
  }
  ++relation_requests_;
  AskLlm("relation",
         BuildRelationRequest(
             fact().text, neighbor().text,
             [this](std::string& input) { llm_engine_->SanitizeInput(input); }),
         base::BindOnce(&DreamingRun::OnLlmRelation,
                        weak_ptr_factory_.GetWeakPtr()));
}

void DreamingRun::OnLlmRelation(std::optional<std::string> answer) {
  std::optional<RelationAnswer> relation =
      answer ? ParseRelationAnswer(*answer) : std::nullopt;
  Trace("llm_relation", base::DictValue()
                            .Set("old", neighbor().text)
                            .Set("answer", CertainToValue(relation)));
  if (!relation) {
    Trace("fact_dropped",
          base::DictValue().Set("reason", "no valid LLM relation answer"));
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
  Trace("relation_applied", base::DictValue()
                                .Set("relation", Name(relation))
                                .Set("old", old.text)
                                .Set("old_type", Name(old.type))
                                .Set("decision_model_certain", certain)
                                .Set("text_change_allowed", can_change));
  if (relation == RelationAnswer::kSame) {
    LearnedMemory updated = old;
    updated.updated_date = std::max(old.updated_date, turn().date);
    AddLinks(updated.links, fact().links);
    Store(std::move(updated), /*is_new=*/false, /*embedding=*/std::nullopt);
    return;
  }
  if (relation == RelationAnswer::kReplace && can_change) {
    LearnedMemory updated = old;
    updated.previous = PreviousMemoryText{old.text, old.links};
    updated.text = fact().text;
    updated.links = fact().links;
    updated.category = fact().category;
    updated.updated_date = turn().date;
    Store(std::move(updated), /*is_new=*/false, fact().vector);
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
        "merge",
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
  const LearnedMemory& old = neighbor();
  // Mechanical guards only, the same as for a rewrite. ParseMergeAnswer()
  // drops an empty text.
  const bool too_long =
      merged && base::UTF8ToUTF16(*merged).size() > kMaxMemoryTextLength;
  Trace("merge_parsed",
        base::DictValue()
            .Set("text", merged ? base::Value(*merged) : base::Value())
            .Set("no_new_details",
                 merged && IsFaithfulRewrite(*merged, {old.text, fact().text},
                                             {turn().date, old.created_date,
                                              old.updated_date}))
            .Set("too_long", too_long));
  if (!merged || too_long) {
    // Both memories stay.
    AddNewMemory();
    return;
  }
  std::string text = *merged;
  Embed("merge", {std::move(*merged)},
        base::BindOnce(&DreamingRun::OnMergedEmbedding,
                       weak_ptr_factory_.GetWeakPtr(), std::move(text)));
}

void DreamingRun::OnMergedEmbedding(std::string merged,
                                    std::vector<std::vector<float>> vectors) {
  const LearnedMemory& old = neighbor();
  LearnedMemory updated = old;
  updated.previous = PreviousMemoryText{old.text, old.links};
  updated.text = std::move(merged);
  AddLinks(updated.links, fact().links);
  updated.updated_date = turn().date;
  Store(std::move(updated), /*is_new=*/false, std::move(vectors[0]));
}

void DreamingRun::AddNewMemory() {
  LearnedMemory memory;
  memory.uuid = base::Uuid::GenerateRandomV4().AsLowercaseString();
  memory.text = fact().text;
  memory.category = fact().category;
  memory.type = fact().type;
  memory.created_date = turn().date;
  memory.updated_date = turn().date;
  memory.last_used_date = turn().date;
  memory.links = fact().links;
  Store(std::move(memory), /*is_new=*/true, fact().vector);
}

void DreamingRun::Store(LearnedMemory memory,
                        bool is_new,
                        std::optional<std::vector<float>> embedding) {
  if (config_.review_changes) {
    // Only the run's copy changes, for the facts that come next.
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(&DreamingRun::OnStored, weak_ptr_factory_.GetWeakPtr(),
                       std::move(memory), is_new, std::move(embedding), true));
    return;
  }
  LearnedMemory copy = memory;
  data_source_->AddOrUpdateLearnedMemory(
      std::move(copy),
      base::BindOnce(&DreamingRun::OnStored, weak_ptr_factory_.GetWeakPtr(),
                     std::move(memory), is_new, std::move(embedding)));
}

void DreamingRun::OnStored(LearnedMemory memory,
                           bool is_new,
                           std::optional<std::vector<float>> embedding,
                           bool success) {
  if (!success) {
    Finish(DreamingStatus::kFailed);
    return;
  }
  if (embedding) {
    run_embeddings_[memory.uuid] = std::move(*embedding);
  }
  if (tracing()) {
    Trace("store", base::DictValue()
                       .Set("action", is_new ? "add" : "update")
                       .Set("text", memory.text)
                       .Set("category", Name(memory.category))
                       .Set("type", Name(memory.type))
                       .Set("previous", memory.previous
                                            ? base::Value(memory.previous->text)
                                            : base::Value())
                       .Set("links", LinksToList(memory.links)));
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

void DreamingRun::AskLlm(std::string_view purpose,
                         MemoryLlmRequest request,
                         LlmCallback callback) {
  StartCall();
  std::string user_message = request.user_message;
  llm_engine_->GenerateMemoryText(
      request.system_prompt, request.user_message,
      base::BindOnce(&DreamingRun::OnLlmAnswer, weak_ptr_factory_.GetWeakPtr(),
                     std::string(purpose), std::move(user_message),
                     std::move(callback)));
}

void DreamingRun::OnLlmAnswer(std::string purpose,
                              std::string user_message,
                              LlmCallback callback,
                              EngineConsumer::GenerationResult result) {
  // A failed LLM request does not stop the run: the step uses its fallback.
  std::optional<std::string> answer;
  if (result.has_value() && result->event &&
      result->event->is_completion_event()) {
    answer = result->event->get_completion_event()->completion;
  }
  Trace("llm_call",
        base::DictValue()
            .Set("purpose", purpose)
            .Set("latency_ms", static_cast<int>(CallMs()))
            .Set("request", user_message)
            .Set("answer", answer ? base::Value(*answer) : base::Value())
            .Set("error",
                 answer ? base::Value()
                        : base::Value(result.has_value() ? "no completion"
                                                         : "request failed")));
  std::move(callback).Run(std::move(answer));
}

void DreamingRun::Embed(std::string_view purpose,
                        std::vector<std::string> passages,
                        EmbeddingsCallback callback) {
  StartCall();
  embed_job_ = embedder_->ComputePassagesEmbeddings(
      passage_embeddings::PassagePriority::kPassive, std::move(passages),
      base::BindOnce(&DreamingRun::OnEmbedded, weak_ptr_factory_.GetWeakPtr(),
                     std::string(purpose), std::move(callback)));
}

void DreamingRun::OnEmbedded(
    std::string purpose,
    EmbeddingsCallback callback,
    std::vector<std::string> passages,
    std::vector<passage_embeddings::Embedding> embeddings,
    uint64_t job_id,
    passage_embeddings::ComputeEmbeddingsStatus status) {
  const bool success =
      status == passage_embeddings::ComputeEmbeddingsStatus::kSuccess &&
      embeddings.size() == passages.size();
  if (tracing()) {
    base::ListValue texts;
    for (const auto& passage : passages) {
      texts.Append(passage);
    }
    Trace("embed",
          base::DictValue()
              .Set("purpose", purpose)
              .Set("latency_ms", static_cast<int>(CallMs()))
              .Set("status", static_cast<int>(status))
              .Set("passages", std::move(texts))
              .Set("dimensions",
                   embeddings.empty()
                       ? 0
                       : static_cast<int>(embeddings[0].GetData().size())));
  }
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
  index_wait_timer_.Stop();
  time_limit_timer_.Stop();
  weak_ptr_factory_.InvalidateWeakPtrs();
  embed_job_.reset();
  result_.status = status;
  if (config_.review_changes && status != DreamingStatus::kCanceled) {
    if (turn_start_memories_) {
      memories_ = std::move(*turn_start_memories_);
      turn_start_memories_.reset();
    }
    result_.review = MakeReview();
    result_.memories_added = 0;
    result_.memories_updated = 0;
    for (const DreamingProposal& proposal : result_.review->proposals) {
      if (proposal.kind == DreamingProposal::Kind::kNew) {
        ++result_.memories_added;
      } else {
        ++result_.memories_updated;
      }
    }
  }
  Trace("done",
        base::DictValue()
            .Set("status", DreamingStatusToString(status))
            .Set("total_ms",
                 static_cast<int>(
                     (base::TimeTicks::Now() - run_start_).InMilliseconds())));
  // The owner can delete this object in |done_|, so this must be the last
  // use of the members.
  std::move(done_).Run(std::move(result_));
}

DreamingReview DreamingRun::MakeReview() const {
  DreamingReview review;
  review.watermarks = review_watermarks_;
  for (const LearnedMemory& memory : memories_) {
    auto stored =
        std::ranges::find(stored_memories_, memory.uuid, &LearnedMemory::uuid);
    DreamingProposal proposal;
    proposal.memory = memory;
    if (stored == stored_memories_.end()) {
      proposal.kind = DreamingProposal::Kind::kNew;
    } else if (*stored == memory) {
      continue;
    } else if (stored->text == memory.text) {
      proposal.kind = DreamingProposal::Kind::kSeenAgain;
      proposal.stored = *stored;
    } else {
      proposal.kind = DreamingProposal::Kind::kChanged;
      // A run can change a memory twice. The text to go back to is the stored
      // one.
      proposal.memory.previous =
          PreviousMemoryText{stored->text, stored->links};
      proposal.stored = *stored;
    }
    review.proposals.push_back(std::move(proposal));
  }
  return review;
}

}  // namespace ai_chat
