// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MEMORY_LLM_PROMPTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MEMORY_LLM_PROMPTS_H_

#include <stddef.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/function_ref.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/browser/memory_decision_client.h"
#include "brave/components/local_ai/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_LOCAL_AI));

// The prompts that Dreaming sends to the local LLM with
// EngineConsumer::GenerateMemoryText(), and the parsers for the answers. The
// LLM never decides what to remember: it only restates, merges, and answers
// the relation question when the decision model is not certain. The
// OpenAI-compatible client sends no response_format, so each prompt asks for
// JSON only, and each parser is strict.
namespace ai_chat {

using SanitizeInputFn = base::FunctionRef<void(std::string&)>;

struct MemoryLlmRequest {
  std::string system_prompt;
  std::string user_message;
};

// A clear fact that the LLM wrote from some sentences of a user turn.
struct RewrittenFact {
  RewrittenFact();
  RewrittenFact(std::string text, std::vector<size_t> sources);
  RewrittenFact(const RewrittenFact&);
  RewrittenFact& operator=(const RewrittenFact&);
  RewrittenFact(RewrittenFact&&);
  RewrittenFact& operator=(RewrittenFact&&);
  ~RewrittenFact();

  bool operator==(const RewrittenFact&) const = default;

  std::string text;
  // Indexes into the sentences of the request. Not empty.
  std::vector<size_t> sources;
};

// Writes the kept sentences of one user turn as clear facts. |turn_date|
// lets the LLM make relative dates ("next month") exact.
MemoryLlmRequest BuildRewriteRequest(std::vector<std::string> sentences,
                                     base::Time turn_date,
                                     SanitizeInputFn sanitize_input);
// Drops facts with no valid source. std::nullopt when the answer is not
// valid.
std::optional<std::vector<RewrittenFact>> ParseRewriteAnswer(
    std::string_view answer,
    size_t sentence_count);

MemoryLlmRequest BuildRelationRequest(std::string new_memory,
                                      std::string old_memory,
                                      SanitizeInputFn sanitize_input);
std::optional<RelationAnswer> ParseRelationAnswer(std::string_view answer);

// Writes one sentence that holds both memories.
MemoryLlmRequest BuildMergeRequest(std::string old_memory,
                                   std::string new_memory,
                                   SanitizeInputFn sanitize_input);
std::optional<std::string> ParseMergeAnswer(std::string_view answer);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_MEMORY_LLM_PROMPTS_H_
