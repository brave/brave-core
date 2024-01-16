// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_claude.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/pattern.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/engine/remote_completion_client.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

namespace {

using mojom::CharacterType;
using mojom::ConversationTurn;

// Marks the beginning of human entries for the model.
// Must be prepended to our prompt, and is appending to the end of Claude prompt
// (as a stop sequence, so it gets stripped).
constexpr char kHumanPromptSequence[] = "\n\nHuman: ";
// Smaller version of the above that we strip from any input text.
constexpr char kHumanPrompt[] = "Human:";

// Marks the beginning of assistant entries for the model
constexpr char kAIPromptSequence[] = "\n\nAssistant: ";
// Smaller version of the above that we strip from any input text.
constexpr char kAIPrompt[] = "Assistant:";

// Produced by our custom prompt:
// (note the blank spaces intentionally added)
constexpr char kHumanPromptPlaceholder[] = "\nH: ";
constexpr char kAIPromptPlaceholder[] = "\n\nA: ";

static constexpr auto kStopSequences =
    base::MakeFixedFlatSet<std::string_view>({kHumanPromptSequence});

std::string GetConversationHistoryString(
    const std::vector<ConversationTurn>& conversation_history) {
  std::vector<std::string> turn_strings;
  for (const ConversationTurn& turn : conversation_history) {
    turn_strings.push_back((turn.character_type == CharacterType::HUMAN
                                ? kHumanPromptPlaceholder
                                : kAIPromptPlaceholder) +
                           turn.text);
  }

  return base::JoinString(turn_strings, "");
}

std::string BuildClaudePrompt(
    const std::string& question_part,
    const std::string& page_content,
    const bool& is_video,
    const std::vector<ConversationTurn>& conversation_history) {
  auto prompt_segment_article =
      page_content.empty()
          ? ""
          : base::StrCat(
                {base::ReplaceStringPlaceholders(
                     l10n_util::GetStringUTF8(
                         is_video ? IDS_AI_CHAT_CLAUDE_VIDEO_PROMPT_SEGMENT
                                  : IDS_AI_CHAT_CLAUDE_ARTICLE_PROMPT_SEGMENT),
                     {page_content}, nullptr),
                 "\n\n"});

  auto prompt_segment_history =
      (conversation_history.empty())
          ? ""
          : base::ReplaceStringPlaceholders(
                l10n_util::GetStringUTF8(
                    IDS_AI_CHAT_CLAUDE_HISTORY_PROMPT_SEGMENT),
                {GetConversationHistoryString(conversation_history)}, nullptr);

  std::string date_and_time_string =
      base::UTF16ToUTF8(TimeFormatFriendlyDateAndTime(base::Time::Now()));
  std::string prompt = base::StrCat(
      {kHumanPromptSequence, prompt_segment_article,
       base::ReplaceStringPlaceholders(
           l10n_util::GetStringUTF8(IDS_AI_CHAT_CLAUDE_SYSTEM_MESSAGE),
           {date_and_time_string, prompt_segment_history, question_part},
           nullptr),
       kAIPromptSequence, " <response>\n"});

  return prompt;
}

void CheckPrompt(std::string& prompt) {
  // TODO(petemill): Perform similar DCHECKs for llama models
  // All queries must have the "Human" and "AI" prompt markers. We do not
  // prepend / append them here since callers may want to put them in
  // custom positions.
  DCHECK(base::MatchPattern(prompt,
                            base::StrCat({"*", kHumanPromptSequence, "*"})));
  DCHECK(
      base::MatchPattern(prompt, base::StrCat({"*", kAIPromptSequence, "*"})));
}

}  // namespace

EngineConsumerClaudeRemote::EngineConsumerClaudeRemote(
    const mojom::Model& model,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager) {
  DCHECK(!model.name.empty());
  base::flat_set<std::string_view> stop_sequences(kStopSequences.begin(),
                                                  kStopSequences.end());
  api_ = std::make_unique<RemoteCompletionClient>(
      model.name, stop_sequences, url_loader_factory, credential_manager);

  max_page_content_length_ = model.max_page_content_length;
}

EngineConsumerClaudeRemote::~EngineConsumerClaudeRemote() = default;

void EngineConsumerClaudeRemote::ClearAllQueries() {
  api_->ClearAllQueries();
}

void EngineConsumerClaudeRemote::GenerateQuestionSuggestions(
    const bool& is_video,
    const std::string& page_content,
    SuggestedQuestionsCallback callback) {
  const std::string& truncated_page_content =
      page_content.substr(0, max_page_content_length_);
  std::string prompt;
  std::vector<std::string> stop_sequences;
  prompt = base::StrCat(
      {kHumanPromptSequence,
       base::ReplaceStringPlaceholders(
           l10n_util::GetStringUTF8(
               is_video ? IDS_AI_CHAT_CLAUDE_VIDEO_PROMPT_SEGMENT
                        : IDS_AI_CHAT_CLAUDE_ARTICLE_PROMPT_SEGMENT),
           {truncated_page_content}, nullptr),
       "\n\n",
       l10n_util::GetStringUTF8(IDS_AI_CHAT_CLAUDE_QUESTION_PROMPT_SEGMENT),
       kAIPromptSequence, "<response>"});
  CheckPrompt(prompt);
  stop_sequences.push_back("</response>");

  api_->QueryPrompt(
      prompt, stop_sequences,
      base::BindOnce(
          &EngineConsumerClaudeRemote::OnGenerateQuestionSuggestionsResponse,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void EngineConsumerClaudeRemote::OnGenerateQuestionSuggestionsResponse(
    SuggestedQuestionsCallback callback,
    GenerationResult result) {
  if (!result.has_value() || result->empty()) {
    // Query resulted in error
    LOG(ERROR) << "Error getting question suggestions.";
    return;
  }

  // Success
  std::vector<std::string> questions =
      base::SplitString(*result, "|", base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);
  std::move(callback).Run(std::move(questions));
}

void EngineConsumerClaudeRemote::GenerateAssistantResponse(
    const bool& is_video,
    const std::string& page_content,
    const ConversationHistory& conversation_history,
    const std::string& human_input,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback) {
  const std::string& truncated_page_content =
      page_content.substr(0, max_page_content_length_);
  std::string prompt = BuildClaudePrompt(human_input, truncated_page_content,
                                         is_video, conversation_history);
  CheckPrompt(prompt);
  api_->QueryPrompt(prompt, {"</response>"}, std::move(completed_callback),
                    std::move(data_received_callback));
}

void EngineConsumerClaudeRemote::SanitizeInput(std::string& input) {
  base::ReplaceSubstringsAfterOffset(&input, 0, kHumanPrompt, "");
  base::ReplaceSubstringsAfterOffset(&input, 0, kAIPrompt, "");
  // TODO(petemill): Do we need to strip the versions of these without newlines?
  base::ReplaceSubstringsAfterOffset(&input, 0, kHumanPromptPlaceholder, "");
  base::ReplaceSubstringsAfterOffset(&input, 0, kAIPromptPlaceholder, "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "<page>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "</page>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "<history>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "</history>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "<question>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "</question>", "");
}

}  // namespace ai_chat
