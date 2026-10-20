// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MEMORY_DECISION_CLIENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MEMORY_DECISION_CLIENT_H_

#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/learned_memory_types.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

namespace ai_chat {

// The answers to one question, each with its probability.
template <typename Answer>
using AnswerProbabilities = base::flat_map<Answer, double>;

// Returns the top answer when it is certain: its probability is at least
// |threshold|, and it is at least |margin| above the next answer. Otherwise
// returns std::nullopt.
template <typename Answer>
std::optional<Answer> GetCertainAnswer(
    const AnswerProbabilities<Answer>& probabilities,
    double threshold,
    double margin) {
  // Tolerance for the floating point subtraction, so that 0.9 - 0.7 is not
  // below a margin of 0.2.
  constexpr double kEpsilon = 1e-9;
  std::optional<Answer> top;
  double top_probability = 0.0;
  double next_probability = 0.0;
  for (const auto& [answer, probability] : probabilities) {
    if (!top || probability > top_probability) {
      if (top) {
        next_probability = top_probability;
      }
      top = answer;
      top_probability = probability;
    } else if (probability > next_probability) {
      next_probability = probability;
    }
  }
  if (!top || top_probability + kEpsilon < threshold ||
      top_probability - next_probability + kEpsilon < margin) {
    return std::nullopt;
  }
  return top;
}

// The answers of the safety question.
enum class SafetyAnswer {
  kOk,
  kSensitive,
  kInstruction,
  kShortLived,
  kNotAboutUser,
};

// How a new memory relates to an old memory.
enum class RelationAnswer {
  // About different things: both stay.
  kDifferent,
  // The same fact: update the date of the old memory.
  kSame,
  // The new memory makes the old memory out of date.
  kReplace,
  // One sentence can hold both.
  kMerge,
  // Both are lines of the same topic. Topics come later, so Dreaming handles
  // this like kDifferent for now.
  kSameTopic,
};

// The answers of the decision model for one sentence of a kept turn.
struct SentenceDecisions {
  SentenceDecisions();
  SentenceDecisions(const SentenceDecisions&);
  SentenceDecisions& operator=(const SentenceDecisions&);
  SentenceDecisions(SentenceDecisions&&);
  SentenceDecisions& operator=(SentenceDecisions&&);
  ~SentenceDecisions();

  // Does the sentence state a lasting fact, preference or topic line?
  AnswerProbabilities<bool> fact;
  AnswerProbabilities<SafetyAnswer> safety;
  AnswerProbabilities<LearnedMemoryCategory> category;
  // Is this a temporary state that ends within weeks?
  AnswerProbabilities<bool> temporary;
};

// Asks the local decision model typed questions, and gives the probability of
// each answer. The model gives no text.
class MemoryDecisionClient {
 public:
  // std::nullopt when the request fails.
  using GateCallback =
      base::OnceCallback<void(std::optional<AnswerProbabilities<bool>>)>;
  // One item for each sentence, in the same order. std::nullopt when the
  // request fails.
  using SentenceDecisionsCallback =
      base::OnceCallback<void(std::optional<std::vector<SentenceDecisions>>)>;
  // One item for each old memory, in the same order. std::nullopt when the
  // request fails.
  using RelationsCallback = base::OnceCallback<void(
      std::optional<std::vector<AnswerProbabilities<RelationAnswer>>>)>;
  // The answers of one relevance request. std::nullopt when the request
  // fails.
  struct RelevanceAnswers {
    RelevanceAnswers();
    RelevanceAnswers(std::vector<double> memories,
                     std::vector<double> preferences);
    RelevanceAnswers(const RelevanceAnswers&);
    RelevanceAnswers(RelevanceAnswers&&);
    RelevanceAnswers& operator=(const RelevanceAnswers&);
    RelevanceAnswers& operator=(RelevanceAnswers&&);
    ~RelevanceAnswers();

    // The probability that each memory is relevant to the message, in the
    // order of the memories.
    std::vector<double> memories;
    // The probability that the answer must follow each preference, in the
    // order of the preferences.
    std::vector<double> preferences;
  };
  using RelevanceCallback =
      base::OnceCallback<void(std::optional<RelevanceAnswers>)>;

  // A Dreaming request to the decision model, as sent, and its answer as
  // received. |response| is null when the request failed, and |error| says
  // why.
  struct Exchange {
    std::string request;
    std::optional<std::string> response;
    std::string error;
    base::TimeDelta latency;
  };
  using ExchangeObserver = base::RepeatingCallback<void(Exchange)>;
  using ExchangeCallback = base::OnceCallback<void(Exchange)>;

  virtual ~MemoryDecisionClient() = default;

  // Gets each Dreaming request and its answer before the answer goes to the
  // caller, so that the run can show them (brave://leo-memory-internals). Chat
  // time requests are not given. A null |observer| stops it.
  virtual void SetDreamingExchangeObserver(ExchangeObserver observer) {}

  // Asks the gate question: does this user turn hold something about the user
  // that can be useful later?
  virtual void AskGate(const std::string& turn_text, GateCallback callback) = 0;

  // Asks the fact, safety, category and type questions for each sentence of
  // one kept turn.
  virtual void AskSentenceDecisions(std::vector<std::string> sentences,
                                    SentenceDecisionsCallback callback) = 0;

  // Asks the relation question for the new memory and each old memory.
  virtual void AskRelations(const std::string& new_memory,
                            std::vector<std::string> old_memories,
                            RelationsCallback callback) = 0;

  // Chat time. Asks, for each memory, if it is relevant to the last user
  // message of a chat, and for each preference, if the answer to the message
  // must follow it. One request has all the questions: the memories first,
  // then the preferences. |previous_message| is the user message before it, or
  // empty. This question comes before the Dreaming questions: while it is
  // active, the client holds back the Dreaming questions that are not sent yet.
  // |on_exchange|, when not null, gets the request and its answer, also when
  // the answer comes after the caller stopped waiting.
  virtual void AskRelevance(const std::string& message,
                            const std::string& previous_message,
                            std::vector<std::string> memories,
                            std::vector<std::string> preferences,
                            RelevanceCallback callback,
                            ExchangeCallback on_exchange) = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MEMORY_DECISION_CLIENT_H_
