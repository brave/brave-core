// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/memory_llm_prompts.h"

#include <algorithm>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"

namespace ai_chat {

RewrittenFact::RewrittenFact() = default;
RewrittenFact::RewrittenFact(std::string text, std::vector<size_t> sources)
    : text(std::move(text)), sources(std::move(sources)) {}
RewrittenFact::RewrittenFact(const RewrittenFact&) = default;
RewrittenFact& RewrittenFact::operator=(const RewrittenFact&) = default;
RewrittenFact::RewrittenFact(RewrittenFact&&) = default;
RewrittenFact& RewrittenFact::operator=(RewrittenFact&&) = default;
RewrittenFact::~RewrittenFact() = default;

namespace {

constexpr char kStyleRules[] =
    "Write each memory as a short fact about the user in the third person, "
    "without a subject, for example \"Lives in Berlin\" or \"Prefers short "
    "answers\". Use only what the input says. Do not guess, and do not add "
    "names, numbers or other details.";

constexpr char kRewritePrompt[] =
    "You turn sentences that a user wrote into clear memories. %s Write one "
    "memory for each idea. Make relative dates (\"next month\", \"just\") "
    "exact with the turn date. For each memory, give the numbers of the "
    "sentences it comes from. The first sentence has the number 0. Answer with "
    "JSON only, in this format: "
    "{\"facts\": [{\"text\": string, \"sources\": [number]}]}. Example: for "
    "one sentence, \"sources\" is [0].";

constexpr char kRelationPrompt[] =
    "You compare two memories about the same user. Answer with JSON only, in "
    "this format: {\"relation\": string}. The relation is one of these: "
    "\"different\" (they are about different things, so both stay), \"same\" "
    "(they say the same thing), \"replace\" (the new memory makes the old "
    "memory out of date, for example a new home replaces an old home), "
    "\"merge\" (the new memory adds to the old memory, so one sentence can "
    "hold both), \"same_topic\" (both are lines of the same ongoing topic). "
    "A temporary state, for example a trip, does not replace a lasting fact.";

constexpr char kMergePrompt[] =
    "You merge two memories about the same user into one sentence. %s Answer "
    "with JSON only, in this format: {\"text\": string}";

constexpr struct {
  const char* name;
  RelationAnswer answer;
} kRelations[] = {
    {"different", RelationAnswer::kDifferent},
    {"same", RelationAnswer::kSame},
    {"replace", RelationAnswer::kReplace},
    {"merge", RelationAnswer::kMerge},
    {"same_topic", RelationAnswer::kSameTopic},
};

std::string FormatDate(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return base::StringPrintf("%04d-%02d-%02d", exploded.year, exploded.month,
                            exploded.day_of_month);
}

// Gets the JSON object in a model answer. The answer can have text or a code
// fence around the object.
std::optional<base::DictValue> ParseJsonAnswer(std::string_view text) {
  size_t start = text.find('{');
  size_t end = text.rfind('}');
  if (start == std::string_view::npos || end == std::string_view::npos ||
      end < start) {
    return std::nullopt;
  }
  return base::JSONReader::ReadDict(text.substr(start, end - start + 1),
                                    base::JSON_PARSE_CHROMIUM_EXTENSIONS);
}

MemoryLlmRequest BuildPairRequest(std::string system_prompt,
                                  std::string old_memory,
                                  std::string new_memory,
                                  SanitizeInputFn sanitize_input) {
  sanitize_input(old_memory);
  sanitize_input(new_memory);
  return {
      std::move(system_prompt),
      base::StrCat({"Old memory: ", old_memory, "\nNew memory: ", new_memory})};
}

}  // namespace

MemoryLlmRequest BuildRewriteRequest(std::vector<std::string> sentences,
                                     base::Time turn_date,
                                     SanitizeInputFn sanitize_input) {
  std::string message = base::StrCat({"Turn date: ", FormatDate(turn_date)});
  for (size_t i = 0; i < sentences.size(); ++i) {
    sanitize_input(sentences[i]);
    base::StrAppend(&message,
                    {"\n", base::NumberToString(i), ": ", sentences[i]});
  }
  return {base::StringPrintf(kRewritePrompt, kStyleRules), std::move(message)};
}

std::optional<std::vector<RewrittenFact>> ParseRewriteAnswer(
    std::string_view answer,
    size_t sentence_count) {
  std::optional<base::DictValue> output = ParseJsonAnswer(answer);
  const base::ListValue* facts = output ? output->FindList("facts") : nullptr;
  if (!facts) {
    return std::nullopt;
  }
  std::vector<RewrittenFact> result;
  for (const auto& item : *facts) {
    const base::DictValue* fact = item.GetIfDict();
    const std::string* text = fact ? fact->FindString("text") : nullptr;
    const base::ListValue* sources = fact ? fact->FindList("sources") : nullptr;
    if (!text || !sources) {
      continue;
    }
    std::string trimmed(base::TrimWhitespaceASCII(*text, base::TRIM_ALL));
    std::vector<size_t> indexes;
    for (const auto& source : *sources) {
      std::optional<int> index = source.GetIfInt();
      if (index && *index >= 0 &&
          static_cast<size_t>(*index) < sentence_count &&
          !std::ranges::contains(indexes, static_cast<size_t>(*index))) {
        indexes.push_back(static_cast<size_t>(*index));
      }
    }
    // A fact without a valid source cannot be checked, so drop it.
    if (trimmed.empty() || indexes.empty()) {
      continue;
    }
    result.emplace_back(std::move(trimmed), std::move(indexes));
  }
  return result;
}

MemoryLlmRequest BuildRelationRequest(std::string new_memory,
                                      std::string old_memory,
                                      SanitizeInputFn sanitize_input) {
  return BuildPairRequest(kRelationPrompt, std::move(old_memory),
                          std::move(new_memory), sanitize_input);
}

std::optional<RelationAnswer> ParseRelationAnswer(std::string_view answer) {
  std::optional<base::DictValue> output = ParseJsonAnswer(answer);
  const std::string* name = output ? output->FindString("relation") : nullptr;
  for (const auto& relation : kRelations) {
    if (name && *name == relation.name) {
      return relation.answer;
    }
  }
  return std::nullopt;
}

MemoryLlmRequest BuildMergeRequest(std::string old_memory,
                                   std::string new_memory,
                                   SanitizeInputFn sanitize_input) {
  return BuildPairRequest(base::StringPrintf(kMergePrompt, kStyleRules),
                          std::move(old_memory), std::move(new_memory),
                          sanitize_input);
}

std::optional<std::string> ParseMergeAnswer(std::string_view answer) {
  std::optional<base::DictValue> output = ParseJsonAnswer(answer);
  const std::string* text = output ? output->FindString("text") : nullptr;
  if (!text) {
    return std::nullopt;
  }
  std::string trimmed(base::TrimWhitespaceASCII(*text, base::TRIM_ALL));
  if (trimmed.empty()) {
    return std::nullopt;
  }
  return trimmed;
}

}  // namespace ai_chat
