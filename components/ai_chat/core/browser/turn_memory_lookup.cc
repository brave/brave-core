// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/turn_memory_lookup.h"

#include <algorithm>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
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

TurnMemoryLookup::TurnMemoryLookup(
    LearnedMemoryDataSource& data_source,
    base::WeakPtr<LearnedMemorySearch> search,
    MemoryDecisionClient& decision_client,
    TurnMemoryConfig config,
    std::vector<std::string> user_messages,
    DoneCallback done,
    MemoryDecisionClient::ExchangeCallback on_exchange)
    : data_source_(data_source),
      search_(std::move(search)),
      decision_client_(decision_client),
      config_(config),
      user_messages_(std::move(user_messages)),
      done_(std::move(done)),
      on_exchange_(std::move(on_exchange)) {
  CHECK(done_);
}

TurnMemoryLookup::~TurnMemoryLookup() = default;

namespace {

// The date of the last mention of a memory.
base::Time LastMentionDate(const LearnedMemory& memory) {
  return memory.updated_date.is_null() ? memory.created_date
                                       : memory.updated_date;
}

}  // namespace

// static
std::string TurnMemoryLookup::FormatForPrompt(const LearnedMemory& memory) {
  const base::Time date = LastMentionDate(memory);
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
                       base::BindOnce(&TurnMemoryLookup::OnTimeout,
                                      weak_ptr_factory_.GetWeakPtr()));
  // The last two user messages, the newest first.
  for (const auto& message : user_messages_) {
    std::string passage(
        base::TruncateUTF8ToByteSize(message, config_.max_message_length));
    if (!base::TrimWhitespaceASCII(passage, base::TRIM_ALL).empty()) {
      passages_.push_back(std::move(passage));
    }
    if (passages_.size() == 2) {
      break;
    }
  }
  base::ListValue messages;
  for (const std::string& passage : passages_) {
    messages.Append(passage);
  }
  Trace("messages", base::DictValue()
                        .Set("messages", std::move(messages))
                        .Set("timeout_ms",
                             static_cast<int>(config_.timeout.InMilliseconds()))
                        .Set("search", !!search_));
  data_source_->GetPermanentLearnedMemories(base::BindOnce(
      &TurnMemoryLookup::OnPermanentMemories, weak_ptr_factory_.GetWeakPtr()));
}

void TurnMemoryLookup::OnPermanentMemories(
    std::vector<LearnedMemory> memories) {
  if (!done_) {
    return;
  }
  permanent_time_ = base::TimeTicks::Now();
  base::ListValue texts;
  for (auto& memory : memories) {
    texts.Append(memory.text);
    result_.permanent_info.push_back({memory.text, LastMentionDate(memory)});
    result_.permanent.push_back(std::move(memory.text));
  }
  Trace("permanent",
        base::DictValue()
            .Set("latency_ms",
                 static_cast<int>(
                     (permanent_time_ - start_time_).InMilliseconds()))
            .Set("memories", std::move(texts)));
  if (!search_ || passages_.empty()) {
    Finish(!search_ ? "no search" : "no message");
    return;
  }
  // The index has the permanent memories too. They are left out of the
  // candidates, so ask for more.
  search_->SearchLearnedMemories(
      passages_, config_.max_candidates + result_.permanent.size(),
      base::BindOnce(&TurnMemoryLookup::OnMatches,
                     weak_ptr_factory_.GetWeakPtr()));
}

void TurnMemoryLookup::OnMatches(std::vector<LearnedMemoryMatch> matches) {
  if (!done_) {
    return;
  }
  searched_time_ = base::TimeTicks::Now();
  match_count_ = matches.size();
  Trace("search",
        base::DictValue()
            .Set("latency_ms",
                 static_cast<int>(
                     (searched_time_ - permanent_time_).InMilliseconds()))
            .Set("found", static_cast<int>(matches.size())));
  if (matches.empty()) {
    Finish("no match");
    return;
  }
  std::vector<std::string> uuids;
  for (auto& match : matches) {
    scores_[match.uuid] = match.score;
    uuids.push_back(std::move(match.uuid));
  }
  data_source_->GetLearnedMemoriesByUuid(
      std::move(uuids), base::BindOnce(&TurnMemoryLookup::OnCandidates,
                                       weak_ptr_factory_.GetWeakPtr()));
}

void TurnMemoryLookup::OnCandidates(std::vector<LearnedMemory> memories) {
  if (!done_) {
    return;
  }
  candidates_time_ = base::TimeTicks::Now();
  // The memories come in the order of the matches, the closest first. A
  // memory deleted after the search is not there.
  for (auto& memory : memories) {
    if (memory.type != LearnedMemoryType::kPermanent &&
        candidates_.size() < config_.max_candidates) {
      candidates_.push_back(std::move(memory));
    }
  }
  base::ListValue candidates;
  for (const LearnedMemory& candidate : candidates_) {
    candidates.Append(base::DictValue()
                          .Set("text", candidate.text)
                          .Set("similarity", scores_[candidate.uuid]));
  }
  Trace("candidates", base::DictValue()
                          .Set("max", static_cast<int>(config_.max_candidates))
                          .Set("candidates", std::move(candidates)));
  if (candidates_.empty()) {
    Finish("no candidate");
    return;
  }
  std::vector<std::string> texts;
  for (const auto& candidate : candidates_) {
    texts.push_back(candidate.text);
  }
  // The message that |passages_| holds first is the newest one.
  decision_client_->AskRelevance(
      passages_[0], passages_.size() > 1 ? passages_[1] : std::string(),
      std::move(texts),
      base::BindOnce(&TurnMemoryLookup::OnRelevance,
                     weak_ptr_factory_.GetWeakPtr()),
      std::move(on_exchange_));
}

void TurnMemoryLookup::OnRelevance(
    std::optional<std::vector<double>> relevance) {
  if (!done_) {
    return;
  }
  if (!relevance || relevance->size() != candidates_.size()) {
    DVLOG(1) << "Learned memory: no relevance answer for the chat turn";
    Finish("relevance request failed");
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
    result_.relevant_info.push_back(
        {candidates_[index].text, LastMentionDate(candidates_[index])});
    relevant_uuids_.push_back(candidates_[index].uuid);
  }
  base::ListValue answers;
  for (size_t i = 0; i < candidates_.size(); ++i) {
    answers.Append(base::DictValue()
                       .Set("text", candidates_[i].text)
                       .Set("relevance", (*relevance)[i])
                       .Set("kept", std::ranges::contains(order, i)));
  }
  Trace("relevance",
        base::DictValue()
            .Set("latency_ms",
                 static_cast<int>((base::TimeTicks::Now() - candidates_time_)
                                      .InMilliseconds()))
            .Set("threshold", config_.relevance_threshold)
            .Set("max_results", static_cast<int>(config_.max_results))
            .Set("memories", std::move(answers)));
  Finish("completed");
}

void TurnMemoryLookup::OnTimeout() {
  Finish("timed out");
}

void TurnMemoryLookup::Trace(std::string_view step, base::DictValue data) {
  data.Set("step", step);
  data.Set("t_ms",
           static_cast<int>(
               (base::TimeTicks::Now() - start_time_).InMilliseconds()));
  trace_.Append(std::move(data));
}

void TurnMemoryLookup::Finish(std::string_view reason) {
  if (!done_) {
    return;
  }
  timeout_timer_.Stop();
  const base::TimeTicks now = base::TimeTicks::Now();
  // The time of each step, or 0 when the step did not end.
  auto step_ms = [](base::TimeTicks from, base::TimeTicks to) {
    return from.is_null() || to.is_null() ? 0 : (to - from).InMilliseconds();
  };
  VLOG(1) << "Learned memory for a chat turn: permanent "
          << step_ms(start_time_, permanent_time_) << " ms, search "
          << step_ms(permanent_time_, searched_time_) << " ms (" << match_count_
          << " found), read " << step_ms(searched_time_, candidates_time_)
          << " ms (" << candidates_.size() << " candidates), relevance "
          << step_ms(candidates_time_, now) << " ms, total "
          << (now - start_time_).InMilliseconds() << " ms, relevant "
          << result_.relevant.size() << ", permanent "
          << result_.permanent.size();
  base::ListValue relevant;
  for (const std::string& memory : result_.relevant) {
    relevant.Append(memory);
  }
  base::ListValue permanent;
  for (const std::string& memory : result_.permanent) {
    permanent.Append(memory);
  }
  Trace("done", base::DictValue()
                    .Set("reason", reason)
                    .Set("total_ms",
                         static_cast<int>((now - start_time_).InMilliseconds()))
                    .Set("relevant", std::move(relevant))
                    .Set("permanent", std::move(permanent)));
  // After a time out, the answers of the steps that are still active are
  // ignored.
  weak_ptr_factory_.InvalidateWeakPtrs();
  std::move(done_).Run(std::move(result_));
}

}  // namespace ai_chat
