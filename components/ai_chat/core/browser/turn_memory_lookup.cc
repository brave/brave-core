// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/turn_memory_lookup.h"

#include <algorithm>
#include <numeric>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/common/features.h"

namespace ai_chat {

// static
TurnMemoryConfig TurnMemoryConfig::FromFeatures() {
  TurnMemoryConfig config;
  config.max_candidates = static_cast<size_t>(
      std::max(1, features::kLearnedMemoryMaxCandidates.Get()));
  config.relevance_threshold = features::kLearnedMemoryRelevanceThreshold.Get();
  config.max_results = static_cast<size_t>(
      std::max(1, features::kLearnedMemoryMaxTurnMemories.Get()));
  config.timeout = features::kLearnedMemoryRelevanceTimeout.Get();
  return config;
}

TurnMemoryLookup::TurnMemoryLookup(base::SequenceBound<AIChatDatabase>& db,
                                   MemoryDecisionClient& decision_client,
                                   passage_embeddings::Embedder& embedder,
                                   TurnMemoryConfig config,
                                   std::vector<std::string> user_messages,
                                   DoneCallback done)
    : db_(db),
      decision_client_(decision_client),
      embedder_(embedder),
      config_(config),
      user_messages_(std::move(user_messages)),
      done_(std::move(done)) {
  CHECK(done_);
}

TurnMemoryLookup::~TurnMemoryLookup() = default;

// static
std::string TurnMemoryLookup::FormatForPrompt(const LearnedMemory& memory) {
  const base::Time date =
      memory.updated_date.is_null() ? memory.created_date : memory.updated_date;
  if (date.is_null()) {
    return memory.text;
  }
  base::Time::Exploded exploded;
  date.UTCExplode(&exploded);
  return base::StringPrintf("%s (%04d-%02d-%02d)", memory.text.c_str(),
                            exploded.year, exploded.month,
                            exploded.day_of_month);
}

void TurnMemoryLookup::Start() {
  start_time_ = base::TimeTicks::Now();
  timeout_timer_.Start(FROM_HERE, config_.timeout,
                       base::BindOnce(&TurnMemoryLookup::Finish,
                                      weak_ptr_factory_.GetWeakPtr()));
  db_->AsyncCall(&AIChatDatabase::GetAllLearnedMemories)
      .Then(base::BindOnce(&TurnMemoryLookup::OnMemories,
                           weak_ptr_factory_.GetWeakPtr()));
}

void TurnMemoryLookup::OnMemories(std::vector<LearnedMemory> memories) {
  if (!done_) {
    return;
  }
  memories_time_ = base::TimeTicks::Now();
  memory_count_ = memories.size();
  for (auto& memory : memories) {
    if (memory.type == LearnedMemoryType::kPermanent) {
      result_.permanent.push_back(std::move(memory.text));
    } else {
      candidates_.push_back(std::move(memory));
    }
  }
  if (candidates_.empty()) {
    Finish();
    return;
  }

  // The last two user messages, the newest first.
  std::vector<std::string> passages;
  for (const auto& message : user_messages_) {
    std::string passage(
        base::TruncateUTF8ToByteSize(message, config_.max_message_length));
    if (!base::TrimWhitespaceASCII(passage, base::TRIM_ALL).empty()) {
      passages.push_back(std::move(passage));
    }
    if (passages.size() == 2) {
      break;
    }
  }
  if (passages.empty()) {
    Finish();
    return;
  }
  embed_job_ = embedder_->ComputePassagesEmbeddings(
      passage_embeddings::PassagePriority::kUserInitiated, std::move(passages),
      base::BindOnce(&TurnMemoryLookup::OnEmbedded,
                     weak_ptr_factory_.GetWeakPtr()));
}

void TurnMemoryLookup::OnEmbedded(
    std::vector<std::string> passages,
    std::vector<passage_embeddings::Embedding> embeddings,
    uint64_t job_id,
    passage_embeddings::ComputeEmbeddingsStatus status) {
  if (!done_) {
    return;
  }
  if (status != passage_embeddings::ComputeEmbeddingsStatus::kSuccess ||
      embeddings.size() != passages.size() || embeddings.empty()) {
    DVLOG(1) << "Learned memory: the embedder failed for the chat turn";
    Finish();
    return;
  }

  embedded_time_ = base::TimeTicks::Now();
  // The similarity of a memory is the best one of the two messages.
  std::vector<float> similarity(candidates_.size(), 0.0f);
  for (size_t i = 0; i < candidates_.size(); ++i) {
    for (const auto& embedding : embeddings) {
      similarity[i] = std::max(
          similarity[i],
          VectorSimilarity(candidates_[i].vector, embedding.GetData()));
    }
  }
  std::vector<size_t> order(candidates_.size());
  std::iota(order.begin(), order.end(), 0);
  std::ranges::stable_sort(
      order, [&](size_t a, size_t b) { return similarity[a] > similarity[b]; });
  order.resize(std::min(order.size(), config_.max_candidates));

  std::vector<LearnedMemory> closest;
  std::vector<std::string> texts;
  for (size_t index : order) {
    texts.push_back(candidates_[index].text);
    closest.push_back(std::move(candidates_[index]));
  }
  candidates_ = std::move(closest);

  // The message that |passages| holds first is the newest one.
  decision_client_->AskRelevance(
      passages[0], passages.size() > 1 ? passages[1] : std::string(),
      std::move(texts),
      base::BindOnce(&TurnMemoryLookup::OnRelevance,
                     weak_ptr_factory_.GetWeakPtr()));
}

void TurnMemoryLookup::OnRelevance(
    std::optional<std::vector<double>> relevance) {
  if (!done_) {
    return;
  }
  if (!relevance || relevance->size() != candidates_.size()) {
    DVLOG(1) << "Learned memory: no relevance answer for the chat turn";
    Finish();
    return;
  }
  std::vector<size_t> order;
  for (size_t i = 0; i < relevance->size(); ++i) {
    if ((*relevance)[i] >= config_.relevance_threshold) {
      order.push_back(i);
    }
  }
  std::ranges::stable_sort(order, [&](size_t a, size_t b) {
    return (*relevance)[a] > (*relevance)[b];
  });
  order.resize(std::min(order.size(), config_.max_results));
  for (size_t index : order) {
    result_.relevant.push_back(FormatForPrompt(candidates_[index]));
  }
  Finish();
}

void TurnMemoryLookup::Finish() {
  if (!done_) {
    return;
  }
  timeout_timer_.Stop();
  embed_job_.reset();
  const base::TimeTicks now = base::TimeTicks::Now();
  VLOG(1) << "Learned memory for a chat turn: " << memory_count_
          << " memories, read "
          << (memories_time_ - start_time_).InMilliseconds() << " ms, embed "
          << (embedded_time_.is_null()
                  ? 0
                  : (embedded_time_ - memories_time_).InMilliseconds())
          << " ms, relevance "
          << (embedded_time_.is_null()
                  ? 0
                  : (now - embedded_time_).InMilliseconds())
          << " ms, total " << (now - start_time_).InMilliseconds()
          << " ms, relevant " << result_.relevant.size() << ", permanent "
          << result_.permanent.size();
  // After a time out, the answers of the steps that are still active are
  // ignored.
  weak_ptr_factory_.InvalidateWeakPtrs();
  std::move(done_).Run(std::move(result_));
}

}  // namespace ai_chat
