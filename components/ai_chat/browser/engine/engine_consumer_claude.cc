// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/browser/engine/engine_consumer_claude.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/pattern.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/browser/ai_chat_api.h"
#include "brave/components/ai_chat/common/features.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

namespace {

using mojom::CharacterType;
using mojom::ConversationTurn;

// Note the blank spaces intentionally added

constexpr char kHumanPromptPlaceholder[] = "\nH: ";
constexpr char kAIPrompt[] = "Assistant:";
constexpr char kAIPromptPlaceholder[] = "\n\nA: ";

std::string GetAssistantPromptSegment() {
  return base::StrCat({"\n\n", kAIPrompt});
}

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
                         is_video ? IDS_AI_CHAT_VIDEO_PROMPT_SEGMENT
                                  : IDS_AI_CHAT_ARTICLE_PROMPT_SEGMENT),
                     {page_content}, nullptr),
                 "\n\n"});

  auto prompt_segment_history =
      (conversation_history.empty())
          ? ""
          : base::ReplaceStringPlaceholders(
                l10n_util::GetStringUTF8(
                    IDS_AI_CHAT_ASSISTANT_HISTORY_PROMPT_SEGMENT),
                {GetConversationHistoryString(conversation_history)}, nullptr);

  std::string prompt = base::StrCat(
      {AIChatAPI::GetHumanPromptSegment(), prompt_segment_article,
       base::ReplaceStringPlaceholders(
           l10n_util::GetStringUTF8(IDS_AI_CHAT_ASSISTANT_PROMPT_SEGMENT),
           {prompt_segment_history, question_part}, nullptr),
       GetAssistantPromptSegment(), " <response>\n"});

  return prompt;
}

void CheckPrompt(std::string& prompt) {
  // TODO(petemill): Perform similar DCHECKs for llama models
  // All queries must have the "Human" and "AI" prompt markers. We do not
  // prepend / append them here since callers may want to put them in
  // custom positions.
  DCHECK(base::MatchPattern(prompt,
                            base::StrCat({"*", kHumanPrompt, "*"})));
  DCHECK(base::MatchPattern(prompt,
                            base::StrCat({"*", kAIPrompt, "*"})));
}

}  // namespace

EngineConsumerClaudeRemote::EngineConsumerClaudeRemote(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  // TODO(petemill): In multi-model mode, this model name will always be a
  // specific claude version - either static or dynamic via some mechanism. Most
  // likley it will be chosen by the server and the general string "claude"
  // provided here.
  const auto model_name = ai_chat::features::kAIModelName.Get();
  api_ = std::make_unique<AIChatAPI>(model_name, url_loader_factory);
}

EngineConsumerClaudeRemote::~EngineConsumerClaudeRemote() = default;

void EngineConsumerClaudeRemote::ClearAllQueries() {
  api_->ClearAllQueries();
}

void EngineConsumerClaudeRemote::GenerateQuestionSuggestions(
    const bool& is_video,
    const std::string& page_content,
    SuggestedQuestionsCallback callback) {
  std::string prompt;
  std::vector<std::string> stop_sequences;
  prompt = base::StrCat(
      {AIChatAPI::GetHumanPromptSegment(),
       base::ReplaceStringPlaceholders(
           l10n_util::GetStringUTF8(is_video
                                        ? IDS_AI_CHAT_VIDEO_PROMPT_SEGMENT
                                        : IDS_AI_CHAT_ARTICLE_PROMPT_SEGMENT),
           {page_content}, nullptr),
       "\n\n", l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_PROMPT_SEGMENT),
       GetAssistantPromptSegment(), " <response>"});
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
    APIRequestResult result) {
  auto success = result.Is2XXResponseCode();
  if (!success) {
    LOG(ERROR) << "Error getting question suggestions. Code: "
               << result.response_code();
    return;
  }
  // Validate
  if (!result.value_body().is_dict()) {
    DVLOG(1) << "Expected dictionary for question suggestion result"
             << " but got: " << result.value_body().DebugString();
    return;
  }
  // TODO(petemill): move common completion basic value lookup to AIChatAPI
  const std::string* completion =
      result.value_body().GetDict().FindString("completion");
  if (!completion || completion->empty()) {
    DVLOG(1) << "Expected completion param for question suggestion"
             << " result but got: " << result.value_body().DebugString();
    return;
  }

  DVLOG(2) << "Received " << (success ? "success" : "failed")
           << " suggested questions response: " << completion;

  std::vector<std::string> questions = base::SplitString(
      *completion, "|", base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);
  std::move(callback).Run(std::move(questions));
}

void EngineConsumerClaudeRemote::SubmitHumanInput(
    const bool& is_video,
    const std::string& page_content,
    const ConversationHistory& conversation_history,
    const std::string& human_input,
    SubmitHumanInputDataReceivedCallback data_received_callback,
    SubmitHumanInputCompletedCallback completed_callback) {
  std::string prompt = BuildClaudePrompt(human_input, page_content, is_video,
                                         conversation_history);
  CheckPrompt(prompt);
  auto on_data_received = base::BindRepeating(
      &EngineConsumerClaudeRemote::OnCompletionDataReceived,
      weak_ptr_factory_.GetWeakPtr(), std::move(data_received_callback));
  auto on_complete = base::BindOnce(
      &EngineConsumerClaudeRemote::OnCompletionCompleted,
      weak_ptr_factory_.GetWeakPtr(), std::move(completed_callback));
  api_->QueryPrompt(prompt, {"</response>"}, std::move(on_complete),
    std::move(on_data_received));
}

void EngineConsumerClaudeRemote::OnCompletionDataReceived(
    SubmitHumanInputDataReceivedCallback callback,
    base::expected<base::Value, std::string> result) {
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  if (const std::string* completion =
          result->GetDict().FindString("completion")) {
    callback.Run(std::move(*completion));
  }
}

void EngineConsumerClaudeRemote::OnCompletionCompleted(
    SubmitHumanInputCompletedCallback callback,
    APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  // Handle successful request
  if (success) {
    std::string completion = "";
    // We're checking for a value body in case for non-streaming API results.
    if (result.value_body().is_dict()) {
      if (const std::string* completion_raw =
              result.value_body().GetDict().FindString("completion")) {
        // Trimming necessary for Llama 2 which prepends responses with a " ".
        completion = base::TrimWhitespaceASCII(*completion_raw, base::TRIM_ALL);
      }
    }
    std::move(callback).Run(base::ok(std::move(completion)));
    return;
  }
  // Handle error
  mojom::APIError error =
      (net::HTTP_TOO_MANY_REQUESTS == result.response_code())
          ? mojom::APIError::RateLimitReached
          : mojom::APIError::ConnectionIssue;
  std::move(callback).Run(base::unexpected(std::move(error)));
}

void EngineConsumerClaudeRemote::SanitizeInput(std::string& input) {
  // Prevent indirect prompt injections being sent to the AI model.
  // Include break-out strings contained in prompts, as well as the base
  // model command separators.
  base::ReplaceSubstringsAfterOffset(&input, 0, kHumanPrompt, "");
  // TODO(petemill): Do we need to strip the versions of these without newlines?
  base::ReplaceSubstringsAfterOffset(&input, 0, kHumanPromptPlaceholder, "");
  base::ReplaceSubstringsAfterOffset(&input, 0, kAIPromptPlaceholder, "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "<article>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "</article>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "<history>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "</history>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "<question>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "</question>", "");
}

}  // namespace ai_chat
