// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_handler.h"

#include <stddef.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/safe_math.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/uuid.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/ai_chat_feedback_api.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "components/grit/brave_components_strings.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

#define STARTER_PROMPT(TYPE)                                              \
  l10n_util::GetStringUTF8(IDS_AI_CHAT_STATIC_STARTER_TITLE_##TYPE),      \
      l10n_util::GetStringUTF8(IDS_AI_CHAT_STATIC_STARTER_PROMPT_##TYPE), \
      mojom::ActionType::CONVERSATION_STARTER

namespace ai_chat {
class AIChatCredentialManager;

namespace {

using ai_chat::mojom::CharacterType;
using ai_chat::mojom::ConversationTurn;

constexpr size_t kDefaultSuggestionsCount = 4;

}  // namespace

ConversationHandler::Suggestion::Suggestion(std::string title)
    : title(std::move(title)) {}
ConversationHandler::Suggestion::Suggestion(std::string title,
                                            std::string prompt)
    : title(std::move(title)), prompt(std::move(prompt)) {}
ConversationHandler::Suggestion::Suggestion(std::string title,
                                            std::string prompt,
                                            mojom::ActionType action_type)
    : title(std::move(title)),
      prompt(std::move(prompt)),
      action_type(action_type) {}
ConversationHandler::Suggestion::Suggestion(Suggestion&&) = default;
ConversationHandler::Suggestion& ConversationHandler::Suggestion::operator=(
    Suggestion&&) = default;
ConversationHandler::Suggestion::~Suggestion() = default;

ConversationHandler::ConversationHandler(
    mojom::Conversation* conversation,
    AIChatService* ai_chat_service,
    ModelService* model_service,
    AIChatCredentialManager* credential_manager,
    AIChatFeedbackAPI* feedback_api,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : ConversationHandler(conversation,
                          ai_chat_service,
                          model_service,
                          credential_manager,
                          feedback_api,
                          url_loader_factory,
                          std::nullopt) {}

ConversationHandler::ConversationHandler(
    mojom::Conversation* conversation,
    AIChatService* ai_chat_service,
    ModelService* model_service,
    AIChatCredentialManager* credential_manager,
    AIChatFeedbackAPI* feedback_api,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::optional<mojom::ConversationArchivePtr> initial_state)
    : associated_content_manager_(
          std::make_unique<AssociatedContentManager>(this)),
      metadata_(conversation),
      ai_chat_service_(ai_chat_service),
      model_service_(model_service),
      credential_manager_(credential_manager),
      feedback_api_(feedback_api),
      url_loader_factory_(url_loader_factory) {
  // When a client disconnects, let observers know
  receivers_.set_disconnect_handler(
      base::BindRepeating(&ConversationHandler::OnClientConnectionChanged,
                          weak_ptr_factory_.GetWeakPtr()));
  conversation_ui_handlers_.set_disconnect_handler(base::BindRepeating(
      &ConversationHandler::OnConversationUIConnectionChanged,
      weak_ptr_factory_.GetWeakPtr()));
  models_observer_.Observe(model_service_.get());

  ChangeModel(metadata_->model_key.value_or("").empty()
                  ? model_service->GetDefaultModelKey()
                  : metadata_->model_key.value());

  if (initial_state.has_value() && !initial_state.value()->entries.empty()) {
    mojom::ConversationArchivePtr conversation_data =
        std::move(initial_state.value());
    if (!conversation_data->associated_content.empty()) {
      associated_content_manager_->LoadArchivedContent(metadata_,
                                                       conversation_data);
    }
    DVLOG(1) << "Restoring associated content for conversation "
             << metadata_->uuid << " with "
             << conversation_data->entries.size();
    chat_history_ = std::move(conversation_data->entries);
  }

  MaybeSeedOrClearSuggestions();
}

ConversationHandler::~ConversationHandler() {
  OnConversationDeleted();
}

void ConversationHandler::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ConversationHandler::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void ConversationHandler::Bind(
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  conversation_ui_handlers_.Add(std::move(conversation_ui_handler));
  OnClientConnectionChanged();

  MaybeFetchOrClearContentStagedConversation();
}

void ConversationHandler::Bind(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  receivers_.Add(this, std::move(receiver));
  Bind(std::move(conversation_ui_handler));
}

void ConversationHandler::Bind(
    mojo::PendingReceiver<mojom::UntrustedConversationHandler> receiver) {
  untrusted_receivers_.Add(this, std::move(receiver));
}

void ConversationHandler::BindUntrustedConversationUI(
    mojo::PendingRemote<mojom::UntrustedConversationUI>
        untrusted_conversation_ui_handler,
    BindUntrustedConversationUICallback callback) {
  auto id = untrusted_conversation_ui_handlers_.Add(
      std::move(untrusted_conversation_ui_handler));
  std::move(callback).Run(GetStateForConversationEntries());

  std::vector<mojom::AssociatedContentPtr> associated_content;
  if (associated_content_manager_->should_send()) {
    untrusted_conversation_ui_handlers_.Get(id)->AssociatedContentChanged(
        associated_content_manager_->GetAssociatedContent());
  }
}

void ConversationHandler::OnArchiveContentUpdated(
    mojom::ConversationArchivePtr conversation_data) {
  UpdateAssociatedContentInfo();
  associated_content_manager_->LoadArchivedContent(metadata_,
                                                   conversation_data);
}

void ConversationHandler::OnAssociatedContentUpdated() {
  UpdateAssociatedContentInfo();
  for (auto& client : conversation_ui_handlers_) {
    client->OnAssociatedContentInfoChanged(
        associated_content_manager_->GetAssociatedContent(),
        associated_content_manager_->should_send());
  }

  OnStateForConversationEntriesChanged();
  MaybeSeedOrClearSuggestions();
  MaybeFetchOrClearContentStagedConversation();

  for (auto& observer : observers_) {
    observer.OnAssociatedContentUpdated(this);
  }

  for (const auto& client : untrusted_conversation_ui_handlers_) {
    if (associated_content_manager_->should_send()) {
      client->AssociatedContentChanged(
          associated_content_manager_->GetAssociatedContent());
    } else {
      client->AssociatedContentChanged({});
    }
  }
}

bool ConversationHandler::IsAnyClientConnected() {
  return !receivers_.empty() || !conversation_ui_handlers_.empty();
}

bool ConversationHandler::HasAnyHistory() {
  if (chat_history_.empty()) {
    return false;
  }
  // If any entry is not staged, then we have history
  return std::ranges::any_of(chat_history_,
                             [](const mojom::ConversationTurnPtr& turn) {
                               return !turn->from_brave_search_SERP;
                             });
}

bool ConversationHandler::IsRequestInProgress() {
  return is_request_in_progress_;
}

bool ConversationHandler::IsAssociatedContentAlive() {
  return associated_content_manager_->HasNonArchiveContent();
}

void ConversationHandler::OnConversationDeleted() {
  for (auto& client : conversation_ui_handlers_) {
    client->OnConversationDeleted();
  }
}

void ConversationHandler::InitEngine() {
  const mojom::Model* model = nullptr;
  if (!model_key_.empty()) {
    model = model_service_->GetModel(model_key_);
  }
  // Make sure we get a valid model, defaulting to static default or first.
  if (!model) {
    // It is unexpected that we get here. Dump a call stack
    // to help figure out why it happens.
    SCOPED_CRASH_KEY_STRING1024("BraveAIChatModel", "key", model_key_);
    base::debug::DumpWithoutCrashing();
    // Use default
    model = model_service_->GetModel(features::kAIModelsDefaultKey.Get());
    if (!model) {
      SCOPED_CRASH_KEY_STRING1024("BraveAIChatModel", "key",
                                  features::kAIModelsDefaultKey.Get());
      base::debug::DumpWithoutCrashing();
      const auto& all_models = model_service_->GetModels();
      // Use first if given bad default value
      model = all_models.at(0).get();
    }
  }

  // Model's key might not be the same as what we asked for (e.g. if the model
  // no longer exists).
  model_key_ = model->key;

  // Update Conversation metadata's model key
  if (model_key_ != model_service_->GetDefaultModelKey()) {
    metadata_->model_key = model_key_;
  } else {
    metadata_->model_key = std::nullopt;
  }

  engine_ = model_service_->GetEngineForModel(model_key_, url_loader_factory_,
                                              credential_manager_);

  OnModelDataChanged();

  if (is_request_in_progress_) {
    // Pending requests have been deleted along with the model engine
    is_request_in_progress_ = false;
    OnAPIRequestInProgressChanged();
  }

  // When the model changes, the content truncation might be different,
  // and the UI needs to know.
  if (associated_content_manager_ &&
      !associated_content_manager_->GetCachedContent().empty()) {
    OnAssociatedContentUpdated();
  }
}

const mojom::Model& ConversationHandler::GetCurrentModel() {
  const mojom::Model* model = model_service_->GetModel(model_key_);
  CHECK(model);
  return *model;
}

const std::vector<mojom::ConversationTurnPtr>&
ConversationHandler::GetConversationHistory() const {
  return chat_history_;
}

void ConversationHandler::GetConversationHistory(
    GetConversationHistoryCallback callback) {
  std::vector<mojom::ConversationTurnPtr> history;
  for (const auto& turn : chat_history_) {
    history.emplace_back(turn->Clone());
  }

  if (pending_conversation_entry_) {
    history.push_back(pending_conversation_entry_->Clone());
  }

  std::move(callback).Run(std::move(history));
}

void ConversationHandler::GetState(GetStateCallback callback) {
  const auto& models = model_service_->GetModels();
  std::vector<mojom::ModelPtr> models_copy(models.size());
  std::transform(models.cbegin(), models.cend(), models_copy.begin(),
                 [](auto& model) { return model.Clone(); });
  auto model_key = GetCurrentModel().key;

  UpdateAssociatedContentInfo();

  std::vector<std::string> suggestions;
  std::ranges::transform(suggestions_, std::back_inserter(suggestions),
                         [](const auto& s) { return s.title; });

  mojom::ConversationStatePtr state = mojom::ConversationState::New(
      metadata_->uuid, is_request_in_progress_, std::move(models_copy),
      model_key, std::move(suggestions), suggestion_generation_status_,
      associated_content_manager_->GetAssociatedContent(),
      associated_content_manager_->should_send(), current_error_,
      metadata_->temporary);

  std::move(callback).Run(std::move(state));
}

void ConversationHandler::RateMessage(bool is_liked,
                                      const std::string& turn_uuid,
                                      RateMessageCallback callback) {
  DVLOG(2) << __func__ << ": " << is_liked << ", " << turn_uuid;

  const std::vector<mojom::ConversationTurnPtr>& history = chat_history_;

  auto entry_it =
      std::ranges::find(history, turn_uuid, &mojom::ConversationTurn::uuid);

  if (entry_it == history.end()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  auto* model = (*entry_it)->model_key
                    ? model_service_->GetModel(*(*entry_it)->model_key)
                    : &GetCurrentModel();
  // Only Leo models are allowed to be rated.
  if (!model || !model->options->is_leo_model_options()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  const size_t count = std::distance(history.begin(), entry_it) + 1;

  base::span<const mojom::ConversationTurnPtr> history_slice =
      base::span(history).first(count);

  feedback_api_->SendRating(
      is_liked, ai_chat_service_->IsPremiumStatus(), history_slice,
      model->options->get_leo_model_options()->name, selected_language_,
      base::BindOnce(
          [](RateMessageCallback callback, APIRequestResult result) {
            if (result.Is2XXResponseCode() && result.value_body().is_dict()) {
              const std::string* id_result =
                  result.value_body().GetDict().FindString("id");
              if (id_result) {
                std::move(callback).Run(*id_result);
              } else {
                DLOG(ERROR) << "Failed to get rating ID";
                std::move(callback).Run(std::nullopt);
              }
              return;
            }
            DLOG(ERROR) << "Failed to send rating: " << result.response_code();
            std::move(callback).Run(std::nullopt);
          },
          std::move(callback)));
}

void ConversationHandler::SendFeedback(const std::string& category,
                                       const std::string& feedback,
                                       const std::string& rating_id,
                                       bool send_hostname,
                                       SendFeedbackCallback callback) {
  DVLOG(2) << __func__ << ": " << rating_id << ", " << send_hostname << ", "
           << category << ", " << feedback;
  auto on_complete = base::BindOnce(
      [](SendFeedbackCallback callback, APIRequestResult result) {
        if (result.Is2XXResponseCode()) {
          std::move(callback).Run(true);
          return;
        }
        DLOG(ERROR) << "Failed to send feedback: " << result.response_code();
        std::move(callback).Run(false);
      },
      std::move(callback));

  std::vector<std::string> urls;
  if (send_hostname) {
    for (const auto& content :
         associated_content_manager_->GetAssociatedContent()) {
      if (!content->url.is_valid() || !content->url.SchemeIsHTTPOrHTTPS()) {
        continue;
      }
      urls.push_back(content->url.host());
    }
  }

  feedback_api_->SendFeedback(
      category, feedback, rating_id,
      urls.empty() ? std::nullopt
                   : std::make_optional(base::JoinString(urls, ",")),
      selected_language_, std::move(on_complete));
}

void ConversationHandler::GetConversationUuid(
    GetConversationUuidCallback callback) {
  std::move(callback).Run(metadata_->uuid);
}

void ConversationHandler::GetModels(GetModelsCallback callback) {
  const auto& models = model_service_->GetModels();
  std::vector<mojom::ModelPtr> models_copy(models.size());
  std::transform(models.cbegin(), models.cend(), models_copy.begin(),
                 [](auto& model) { return model.Clone(); });
  std::move(callback).Run(std::move(models_copy), GetCurrentModel().key);
}

void ConversationHandler::ChangeModel(const std::string& model_key) {
  CHECK(!model_key.empty());
  // Check that the key exists
  auto* new_model = model_service_->GetModel(model_key);
  if (new_model) {
    model_key_ = new_model->key;

    // Applies to Custom Models alone. Verify that the endpoint URL for this
    // model is valid. Model endpoints may be valid in one session, but not in
    // another. For example, if --allow-leo-private-ips is enabled, the endpoint
    // does not need to use HTTPS.
    if (new_model->options->is_custom_model_options()) {
      const bool is_valid_endpoint = ModelValidator::IsValidEndpoint(
          new_model->options->get_custom_model_options()->endpoint);
      SetAPIError(is_valid_endpoint ? mojom::APIError::None
                                    : mojom::APIError::InvalidEndpointURL);
    } else {
      // Non-custom model activated; clear any previous API error.
      SetAPIError(mojom::APIError::None);
    }
  }

  // Always call InitEngine, even with a bad key as we need a model
  InitEngine();
}

void ConversationHandler::GetIsRequestInProgress(
    GetIsRequestInProgressCallback callback) {
  std::move(callback).Run(is_request_in_progress_);
}

void ConversationHandler::SubmitHumanConversationEntry(
    const std::string& input,
    std::optional<std::vector<mojom::UploadedFilePtr>> uploaded_files) {
  DCHECK(!is_request_in_progress_)
      << "Should not be able to submit more"
      << "than a single human conversation turn at a time.";

  if (uploaded_files && !uploaded_files->empty() &&
      std::ranges::any_of(
          uploaded_files.value(), [](const mojom::UploadedFilePtr& file) {
            return file->type == mojom::UploadedFileType::kImage ||
                   file->type == mojom::UploadedFileType::kScreenshot;
          })) {
    auto* current_model =
        model_service_->GetModel(metadata_->model_key.value_or("").empty()
                                     ? model_service_->GetDefaultModelKey()
                                     : metadata_->model_key.value());
    if (!current_model->vision_support) {
      ChangeModel(ai_chat_service_->IsPremiumStatus()
                      ? features::kAIModelsPremiumVisionDefaultKey.Get()
                      : features::kAIModelsVisionDefaultKey.Get());
    }
  }
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      std::nullopt, CharacterType::HUMAN, mojom::ActionType::QUERY, input,
      std::nullopt /* prompt */, std::nullopt /* selected_text */,
      std::nullopt /* events */, base::Time::Now(), std::nullopt /* edits */,
      std::move(uploaded_files), false, std::nullopt /* model_key */);
  SubmitHumanConversationEntry(std::move(turn));
}

void ConversationHandler::SubmitHumanConversationEntry(
    mojom::ConversationTurnPtr turn) {
  VLOG(1) << __func__;
  DVLOG(4) << __func__ << ": " << turn->text;

  // If there's edits, use the last one as the latest turn.
  bool has_edits = turn->edits && !turn->edits->empty();
  mojom::ConversationTurnPtr& latest_turn =
      has_edits ? turn->edits->back() : turn;
  // Decide if this entry needs to wait for one of:
  // - user to be opted-in
  // - is request in progress (should only be possible if regular entry is
  // in-progress and another entry is submitted outside of regular UI, e.g. from
  // location bar or context menu.
  if (!ai_chat_service_->HasUserOptedIn() || is_request_in_progress_) {
    VLOG(1) << "Adding as a pending conversation entry";
    // This is possible (on desktop) if user submits multiple location bar
    // messages before an entry is complete. But that should be obvious from the
    // UI that the 1 in-progress + 1 pending message is the limit.
    if (pending_conversation_entry_) {
      VLOG(1) << "Should not be able to add a pending conversation entry "
              << "when there is already a pending conversation entry.";
      return;
    }
    pending_conversation_entry_ = std::move(turn);
    // Pending entry is added to conversation history when asked for
    // so notify observers.
    OnHistoryUpdate(nullptr);
    return;
  }
  DCHECK(latest_turn->character_type == mojom::CharacterType::HUMAN);
  is_request_in_progress_ = true;
  OnAPIRequestInProgressChanged();

  // Directly modify Entry's text to remove engine-breaking substrings
  if (!has_edits) {  // Edits are already sanitized.
    engine_->SanitizeInput(latest_turn->text);
  }
  if (latest_turn->selected_text) {
    engine_->SanitizeInput(*latest_turn->selected_text);
  }

  // If it's a suggested question, replace with the prompt.
  std::string prompt = latest_turn->prompt.value_or(latest_turn->text);

  // Add the human part to the conversation
  AddToConversationHistory(std::move(turn));
  const bool is_page_associated =
      associated_content_manager_->HasAssociatedContent() &&
      associated_content_manager_->should_send();
  if (is_page_associated) {
    // Fetch updated page content before performing generation
    GeneratePageContent(
        base::BindOnce(&ConversationHandler::PerformAssistantGeneration,
                       weak_ptr_factory_.GetWeakPtr()));
  } else {
    // Now the conversation is committed, we can remove some unneccessary data
    // if we're not associated with a page.
    suggestions_.clear();

    // Reset the content to be empty.
    associated_content_manager_->AddContent(nullptr, /*notify_updated=*/true,
                                            /*detach_existing_content=*/true);

    OnSuggestedQuestionsChanged();
    // Perform generation immediately
    PerformAssistantGeneration();
  }
}

void ConversationHandler::SubmitHumanConversationEntryWithAction(
    const std::string& input,
    mojom::ActionType action_type) {
  DCHECK(!is_request_in_progress_)
      << "Should not be able to submit more"
      << "than a single human conversation turn at a time.";

  SubmitSelectedText(input, action_type);
}

void ConversationHandler::ModifyConversation(uint32_t turn_index,
                                             const std::string& new_text) {
  if (turn_index >= chat_history_.size()) {
    return;
  }

  auto& turn = chat_history_.at(turn_index);
  // Modifying answer, create an entry in edits with updated completion event.
  if (turn->character_type == CharacterType::ASSISTANT) {
    if (!turn->events || turn->events->empty()) {
      return;
    }

    std::optional<size_t> completion_event_index;
    for (size_t i = 0; i < turn->events->size(); ++i) {
      if (turn->events->at(i)->is_completion_event()) {
        completion_event_index = i;
        break;
      }
    }
    if (!completion_event_index.has_value()) {
      return;
    }

    std::string trimmed_input;
    base::TrimWhitespaceASCII(new_text, base::TRIM_ALL, &trimmed_input);
    if (trimmed_input.empty() ||
        trimmed_input == turn->events->at(*completion_event_index)
                             ->get_completion_event()
                             ->completion) {
      return;
    }

    std::vector<mojom::ConversationEntryEventPtr> events;
    for (auto& event : *turn->events) {
      events.push_back(event->Clone());
    }

    auto edited_turn = mojom::ConversationTurn::New(
        base::Uuid::GenerateRandomV4().AsLowercaseString(),
        turn->character_type, turn->action_type, trimmed_input,
        std::nullopt /* prompt */, std::nullopt /* selected_text */,
        std::move(events), base::Time::Now(), std::nullopt /* edits */,
        std::nullopt, false, turn->model_key);
    edited_turn->events->at(*completion_event_index)
        ->get_completion_event()
        ->completion = trimmed_input;

    if (!turn->edits) {
      turn->edits.emplace();
    }
    turn->edits->emplace_back(std::move(edited_turn));

    OnConversationEntryRemoved(turn->uuid);
    OnConversationEntryAdded(turn);

    return;
  }

  // Modifying human turn, create an entry in edits with updated text, drop
  // anything after this turn_index and resubmit.
  std::string sanitized_input = new_text;
  engine_->SanitizeInput(sanitized_input);
  const auto& current_text = turn->edits && !turn->edits->empty()
                                 ? turn->edits->back()->text
                                 : turn->text;
  if (sanitized_input.empty() || sanitized_input == current_text) {
    return;
  }

  // turn->selected_text and turn->events are actually std::nullopt for
  // editable human turns in our current implementation, just use std::nullopt
  // here directly to be more explicit and avoid confusion.
  auto edited_turn = mojom::ConversationTurn::New(
      base::Uuid::GenerateRandomV4().AsLowercaseString(), turn->character_type,
      turn->action_type, sanitized_input, std::nullopt /* prompt */,
      std::nullopt /* selected_text */, std::nullopt /* events */,
      base::Time::Now(), std::nullopt /* edits */, std::nullopt, false,
      turn->model_key);
  if (!turn->edits) {
    turn->edits.emplace();
  }
  // Erase all turns after the edited turn and notify observers
  std::vector<std::optional<std::string>> erased_turn_ids;
  std::ranges::transform(
      chat_history_.begin() + turn_index, chat_history_.end(),
      std::back_inserter(erased_turn_ids),
      [](mojom::ConversationTurnPtr& turn) { return turn->uuid; });
  turn->edits->emplace_back(std::move(edited_turn));
  auto new_turn = std::move(chat_history_.at(turn_index));
  chat_history_.erase(chat_history_.begin() + turn_index, chat_history_.end());

  for (auto& uuid : erased_turn_ids) {
    OnConversationEntryRemoved(uuid);
  }

  SubmitHumanConversationEntry(std::move(new_turn));
}

void ConversationHandler::RegenerateAnswer(const std::string& turn_uuid,
                                           const std::string& model_key) {
  // Find the turn with the given UUID.
  auto turn_it = std::ranges::find_if(
      chat_history_,
      [&turn_uuid](const auto& turn) { return turn->uuid == turn_uuid; });
  auto turn_pos = std::distance(chat_history_.begin(), turn_it);

  // Validate that we found a valid position, it's not the first turn, and it's
  // an assistant turn.
  if (turn_it == chat_history_.end() || turn_pos == 0 ||
      (*turn_it)->character_type != CharacterType::ASSISTANT) {
    return;
  }

  // The question is at the position before the found turn.
  size_t question_pos = turn_pos - 1;

  // Create a span from the question turn to the end.
  auto turns_to_erase = base::span(chat_history_).subspan(question_pos);

  // Collect the UUIDs of turns that will be erased.
  std::vector<std::optional<std::string>> erased_turn_ids;
  std::ranges::transform(
      turns_to_erase, std::back_inserter(erased_turn_ids),
      [](const mojom::ConversationTurnPtr& turn) { return turn->uuid; });

  auto question_turn = std::move(turns_to_erase.front());
  // Specify model key to be used for generating the answer.
  question_turn->model_key = model_key;

  // Erase the question turn in history and everything after it.
  chat_history_.erase(chat_history_.begin() + question_pos,
                      chat_history_.end());
  for (const auto& uuid : erased_turn_ids) {
    OnConversationEntryRemoved(uuid);
  }

  // Submit the question turn to generate a new answer.
  SubmitHumanConversationEntry(std::move(question_turn));
}

void ConversationHandler::SubmitSummarizationRequest() {
  // This is a special case for the pre-optin UI which has a specific button
  // to summarize the page if we're associatable with content.
  DCHECK(associated_content_manager_->HasAssociatedContent())
      << "This conversation request is not associated with content";
  DCHECK(associated_content_manager_->should_send())
      << "This conversation request should send page contents";

  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      std::nullopt, CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_PAGE,
      l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE),
      l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE),
      std::nullopt /* selected_text */, std::nullopt /* events */,
      base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_images */, false, std::nullopt /* model_key */);
  SubmitHumanConversationEntry(std::move(turn));
}

void ConversationHandler::SubmitSuggestion(
    const std::string& suggestion_title) {
  if (is_request_in_progress_) {
    DLOG(ERROR) << "UI should not allow submitting a suggestion when a "
                   "previous request is in progress.";
    return;
  }

  auto suggest_it =
      std::ranges::find(suggestions_, suggestion_title, &Suggestion::title);
  if (suggest_it == suggestions_.end()) {
    DLOG(ERROR)
        << "A suggestion was submitted that is not in the suggestions list.";
    return;
  }

  Suggestion& suggestion = *suggest_it;

  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      std::nullopt, CharacterType::HUMAN, suggestion.action_type,
      suggestion.title, suggestion.prompt, std::nullopt /* selected_text */,
      std::nullopt /* events */, base::Time::Now(), std::nullopt /* edits */,
      std::nullopt, false, std::nullopt /* model_key */);
  SubmitHumanConversationEntry(std::move(turn));

  // Remove the suggestion from the list, assume the list has been modified
  // inside SubmitHumanConversationEntry so search for it again.
  auto to_remove =
      std::ranges::remove(suggestions_, suggestion_title, &Suggestion::title);
  suggestions_.erase(to_remove.begin(), to_remove.end());
  OnSuggestedQuestionsChanged();
}

std::vector<std::string> ConversationHandler::GetSuggestedQuestionsForTest() {
  std::vector<std::string> suggestions;
  std::ranges::transform(suggestions_, std::back_inserter(suggestions),
                         [](const auto& s) { return s.title; });
  return suggestions;
}

void ConversationHandler::SetSuggestedQuestionForTest(std::string title,
                                                      std::string prompt) {
  suggestions_.clear();
  suggestions_.emplace_back(title, prompt);
}

void ConversationHandler::GenerateQuestions() {
  DVLOG(1) << __func__;
  // This function should not be presented in the UI if the user has not
  // opted-in yet.
  if (!ai_chat_service_->HasUserOptedIn()) {
    DLOG(ERROR) << "GenerateQuestions should not be called before user is "
                << "opted in to AI Chat";
    return;
  }
  if (!associated_content_manager_->should_send()) {
    DLOG(ERROR) << "Cannot get suggestions when not associated with content.";
    return;
  }
  if (!associated_content_manager_->HasAssociatedContent()) {
    DLOG(ERROR)
        << "Should not be associated with content when not allowed to be";
    return;
  }
  // We're not expecting to call this if the UI is not active for this
  // conversation.
  if (!IsAnyClientConnected()) {
    DLOG(ERROR) << "GenerateQuestions should not be called when no clients are "
                   "connected to this conversation";
    return;
  }
  // We're not expecting to already have generated suggestions
  const size_t expected_existing_suggestions_size =
      (associated_content_manager_->HasAssociatedContent() &&
       associated_content_manager_->should_send())
          ? 1u
          : 0u;
  if (suggestions_.size() > expected_existing_suggestions_size) {
    DLOG(ERROR) << "GenerateQuestions should not be called more than once";
    return;
  }

  if (suggestion_generation_status_ ==
          mojom::SuggestionGenerationStatus::IsGenerating ||
      suggestion_generation_status_ ==
          mojom::SuggestionGenerationStatus::HasGenerated) {
    DLOG(ERROR) << "UI should not allow GenerateQuestions to be called more "
                << "than once";
    return;
  }

  suggestion_generation_status_ =
      mojom::SuggestionGenerationStatus::IsGenerating;
  OnSuggestedQuestionsChanged();
  // Make API request for questions but first get page content.
  // Do not call SetRequestInProgress, this progress
  // does not need to be shown to the UI.
  GeneratePageContent(
      base::BindOnce(&ConversationHandler::PerformQuestionGeneration,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ConversationHandler::PerformQuestionGeneration(
    std::string page_content,
    bool is_video,
    std::string invalidation_token) {
  engine_->GenerateQuestionSuggestions(
      is_video, page_content, selected_language_,
      base::BindOnce(&ConversationHandler::OnSuggestedQuestionsResponse,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ConversationHandler::GetAssociatedContentInfo(
    GetAssociatedContentInfoCallback callback) {
  UpdateAssociatedContentInfo();
  std::move(callback).Run(associated_content_manager_->GetAssociatedContent(),
                          associated_content_manager_->should_send());
}

void ConversationHandler::SetShouldSendPageContents(bool should_send) {
  if (associated_content_manager_->should_send() == should_send) {
    return;
  }
  if (!associated_content_manager_->HasAssociatedContent() && should_send) {
    return;
  }
  associated_content_manager_->SetShouldSend(should_send);

  MaybeSeedOrClearSuggestions();
  MaybeFetchOrClearContentStagedConversation();
}

void ConversationHandler::RetryAPIRequest() {
  SetAPIError(mojom::APIError::None);
  DCHECK(!chat_history_.empty());

  // We're using a reverse iterator here to find the latest human turn
  for (std::vector<mojom::ConversationTurnPtr>::reverse_iterator rit =
           chat_history_.rbegin();
       rit != chat_history_.rend(); ++rit) {
    if (rit->get()->character_type == CharacterType::HUMAN) {
      auto turn = *std::make_move_iterator(rit);
      auto human_turn_iter = rit.base() - 1;
      chat_history_.erase(human_turn_iter, chat_history_.end());
      SubmitHumanConversationEntry(std::move(turn));
      break;
    }
  }
}

void ConversationHandler::GetAPIResponseError(
    GetAPIResponseErrorCallback callback) {
  std::move(callback).Run(current_error_);
}

void ConversationHandler::StopGenerationAndMaybeGetHumanEntry(
    StopGenerationAndMaybeGetHumanEntryCallback callback) {
  if (chat_history_.empty()) {
    std::move(callback).Run(nullptr);
    return;
  }

  is_request_in_progress_ = false;
  engine_->ClearAllQueries();
  OnAPIRequestInProgressChanged();

  mojom::CharacterType last_type = chat_history_.back()->character_type;
  if (last_type == mojom::CharacterType::HUMAN) {
    mojom::ConversationTurnPtr turn = std::move(chat_history_.back());
    chat_history_.pop_back();
    OnConversationEntryRemoved(turn->uuid);
    std::move(callback).Run(std::move(turn));
  } else {
    std::move(callback).Run(nullptr);
  }
}

void ConversationHandler::ClearErrorAndGetFailedMessage(
    ClearErrorAndGetFailedMessageCallback callback) {
  DCHECK(!chat_history_.empty());

  SetAPIError(mojom::APIError::None);
  mojom::ConversationTurnPtr turn = std::move(chat_history_.back());
  chat_history_.pop_back();

  OnConversationEntryRemoved(turn->uuid);

  std::move(callback).Run(std::move(turn));
}

void ConversationHandler::SubmitSelectedText(const std::string& selected_text,
                                             mojom::ActionType action_type) {
  const std::string& question = GetActionTypeQuestion(action_type);
  SubmitSelectedTextWithQuestion(selected_text, question, action_type);
}

void ConversationHandler::SubmitSelectedTextWithQuestion(
    const std::string& selected_text,
    const std::string& question,
    mojom::ActionType action_type) {
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      std::nullopt, CharacterType::HUMAN, action_type, question,
      std::nullopt /* prompt */, selected_text, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, false, std::nullopt /* model_key */);

  SubmitHumanConversationEntry(std::move(turn));
}

bool ConversationHandler::MaybePopPendingRequests() {
  if (!ai_chat_service_->HasUserOptedIn()) {
    return false;
  }

  if (!pending_conversation_entry_) {
    return false;
  }

  mojom::ConversationTurnPtr request = std::move(pending_conversation_entry_);
  pending_conversation_entry_.reset();
  SubmitHumanConversationEntry(std::move(request));
  return true;
}

void ConversationHandler::MaybeUnlinkAssociatedContent() {
  // Only unlink if panel is closed and there is no conversation history.
  // When panel is open or has existing conversation, do not change the state.
  if (chat_history_.empty() && !IsAnyClientConnected()) {
    SetShouldSendPageContents(false);
  }
}

void ConversationHandler::AddSubmitSelectedTextError(
    const std::string& selected_text,
    mojom::ActionType action_type,
    mojom::APIError error) {
  if (error == mojom::APIError::None) {
    return;
  }
  const std::string& question = GetActionTypeQuestion(action_type);
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      std::nullopt, CharacterType::HUMAN, action_type, question,
      std::nullopt /* prompt */, selected_text, std::nullopt, base::Time::Now(),
      std::nullopt, std::nullopt, false, std::nullopt /* model_key */);
  AddToConversationHistory(std::move(turn));
  SetAPIError(error);
}

void ConversationHandler::OnUserOptedIn() {
  MaybePopPendingRequests();
  MaybeFetchOrClearContentStagedConversation();
}

void ConversationHandler::AddToConversationHistory(
    mojom::ConversationTurnPtr turn) {
  if (!turn) {
    return;
  }

  if (!turn->uuid.has_value()) {
    turn->uuid = base::Uuid::GenerateRandomV4().AsLowercaseString();
  }

  chat_history_.push_back(std::move(turn));

  OnConversationEntryAdded(chat_history_.back());
}

void ConversationHandler::PerformAssistantGeneration(
    std::string page_content /* = "" */,
    bool is_video /* = false */,
    std::string invalidation_token /* = "" */) {
  if (chat_history_.empty()) {
    DLOG(ERROR) << "Cannot generate assistant response without any history";
    return;
  }

  engine_->GenerateAssistantResponse(
      is_video, page_content, chat_history_, selected_language_,
      base::BindRepeating(&ConversationHandler::OnEngineCompletionDataReceived,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindOnce(&ConversationHandler::OnEngineCompletionComplete,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ConversationHandler::SetAPIError(const mojom::APIError& error) {
  current_error_ = error;

  for (auto& client : conversation_ui_handlers_) {
    client->OnAPIResponseError(error);
  }
}

void ConversationHandler::UpdateOrCreateLastAssistantEntry(
    EngineConsumer::GenerationResultData result) {
  if (chat_history_.empty() ||
      chat_history_.back()->character_type != CharacterType::ASSISTANT) {
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New(
        base::Uuid::GenerateRandomV4().AsLowercaseString(),
        CharacterType::ASSISTANT, mojom::ActionType::RESPONSE, "",
        std::nullopt /* prompt */, std::nullopt,
        std::vector<mojom::ConversationEntryEventPtr>{}, base::Time::Now(),
        std::nullopt, std::nullopt, false, result.model_key);
    chat_history_.push_back(std::move(entry));
  }

  auto& entry = chat_history_.back();
  auto& event = result.event;
  if (event->is_completion_event()) {
    if (!engine_->SupportsDeltaTextResponses() || entry->events->size() == 0 ||
        !entry->events->back()->is_completion_event()) {
      // The start of completion responses needs whitespace trim
      // TODO(petemill): This should happen server-side?
      event->get_completion_event()->completion = base::TrimWhitespaceASCII(
          event->get_completion_event()->completion, base::TRIM_LEADING);
    }

    // Optimize by merging with previous completion events if delta updates
    // are supported or otherwise replacing the previous event.
    if (entry->events->size() > 0) {
      auto& last_event = entry->events->back();
      if (last_event->is_completion_event()) {
        // Merge completion events
        if (engine_->SupportsDeltaTextResponses()) {
          event->get_completion_event()->completion =
              base::StrCat({last_event->get_completion_event()->completion,
                            event->get_completion_event()->completion});
        }
        // Remove the last event because we'll replace in both delta and
        // non-delta cases
        entry->events->pop_back();
      }
    }

    // TODO(petemill): Remove ConversationTurn.text backwards compatibility when
    // all UI is updated to instead use ConversationEntryEvent items.
    entry->text = event->get_completion_event()->completion;
  }

  if (event->is_conversation_title_event()) {
    OnConversationTitleChanged(event->get_conversation_title_event()->title);
    // Don't add this event to history
    return;
  }

  if (event->is_selected_language_event()) {
    OnSelectedLanguageChanged(
        event->get_selected_language_event()->selected_language);
    // Don't add this event to history
    return;
  }

  if (event->is_content_receipt_event()) {
    OnConversationTokenInfoChanged(
        event->get_content_receipt_event()->total_tokens,
        event->get_content_receipt_event()->trimmed_tokens);
    // Don't add this event to history
    return;
  }

  entry->events->push_back(std::move(event));
  // Update clients for partial entries but not observers, who will get notified
  // when we know this is a complete entry.
  OnHistoryUpdate(entry.Clone());
}

void ConversationHandler::MaybeSeedOrClearSuggestions() {
  const bool is_page_associated =
      associated_content_manager_->HasAssociatedContent() &&
      associated_content_manager_->should_send();

  if (!is_page_associated) {
    suggestions_.clear();
    suggestion_generation_status_ = mojom::SuggestionGenerationStatus::None;
    if (!chat_history_.empty()) {
      return;
    }

    suggestions_.emplace_back(STARTER_PROMPT(MEMO));
    suggestions_.emplace_back(STARTER_PROMPT(INTERVIEW));
    suggestions_.emplace_back(STARTER_PROMPT(STUDY_PLAN));
    suggestions_.emplace_back(STARTER_PROMPT(PROJECT_TIMELINE));
    suggestions_.emplace_back(STARTER_PROMPT(MARKETING_STRATEGY));
    suggestions_.emplace_back(STARTER_PROMPT(PRESENTATION_OUTLINE));
    suggestions_.emplace_back(STARTER_PROMPT(BRAINSTORM));
    suggestions_.emplace_back(STARTER_PROMPT(PROFESSIONAL_EMAIL));
    suggestions_.emplace_back(STARTER_PROMPT(BUSINESS_PROPOSAL));

    // We don't have an external list of all the available suggestions, so we
    // generate all of them  and remove random ones until we have the required
    // number and then shuffle the result.
    while (suggestions_.size() > kDefaultSuggestionsCount) {
      auto remove_at = base::RandInt(0, suggestions_.size() - 1);
      suggestions_.erase(suggestions_.begin() + remove_at);
    }
    base::RandomShuffle(suggestions_.begin(), suggestions_.end());
    OnSuggestedQuestionsChanged();
    return;
  }

  // This means we have the default suggestions
  if (suggestion_generation_status_ ==
      mojom::SuggestionGenerationStatus::None) {
    suggestions_.clear();
  }

  if (suggestions_.empty() &&
      suggestion_generation_status_ !=
          mojom::SuggestionGenerationStatus::IsGenerating &&
      suggestion_generation_status_ !=
          mojom::SuggestionGenerationStatus::HasGenerated) {
    // TODO(petemill): ask content fetcher if it knows whether current page is a
    // video.
    auto found_iter = std::ranges::find_if(
        chat_history_, [](mojom::ConversationTurnPtr& turn) {
          if (turn->action_type == mojom::ActionType::SUMMARIZE_PAGE ||
              turn->action_type == mojom::ActionType::SUMMARIZE_VIDEO) {
            return true;
          }
          return false;
        });
    const bool has_summarized = found_iter != chat_history_.end();
    if (!has_summarized) {
      if (associated_content_manager_->IsVideo()) {
        suggestions_.emplace_back(
            l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_VIDEO),
            l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_VIDEO),
            mojom::ActionType::SUMMARIZE_VIDEO);
      } else {
        suggestions_.emplace_back(
            l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE),
            l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE),
            mojom::ActionType::SUMMARIZE_PAGE);
      }
    }
    suggestion_generation_status_ =
        mojom::SuggestionGenerationStatus::CanGenerate;
    OnSuggestedQuestionsChanged();
  } else if (!suggestions_.empty() &&
             suggestions_[0].action_type == mojom::ActionType::SUMMARIZE_PAGE) {
    // The title for the summarize page suggestion needs to be updated when
    // the number of associated content items changes.
    suggestions_[0].title =
        l10n_util::GetPluralStringFUTF8(IDS_CHAT_UI_SUMMARIZE_PAGES_SUGGESTION,
                                        metadata_->associated_content.size());
    OnSuggestedQuestionsChanged();
  }
}

void ConversationHandler::MaybeFetchOrClearContentStagedConversation() {
  // Try later when we get a connected client
  if (!IsAnyClientConnected()) {
    return;
  }

  const bool can_check_for_staged_conversation =
      associated_content_manager_->HasAssociatedContent() &&
      associated_content_manager_->should_send();
  if (!can_check_for_staged_conversation) {
    // Clear any staged conversation entries since user might have unassociated
    // content with this conversation
    size_t num_entries = chat_history_.size();
    std::erase_if(chat_history_, [](const mojom::ConversationTurnPtr& turn) {
      return turn->from_brave_search_SERP;
    });
    if (num_entries != chat_history_.size()) {
      OnHistoryUpdate(nullptr);
    }
    return;
  }

  associated_content_manager_->GetStagedEntriesFromContent(
      base::BindOnce(&ConversationHandler::OnGetStagedEntriesFromContent,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ConversationHandler::OnGetStagedEntriesFromContent(
    const std::optional<std::vector<SearchQuerySummary>>& entries) {
  // Check if all requirements are still met.
  if (is_request_in_progress_ || !entries ||
      !associated_content_manager_->HasAssociatedContent() ||
      !associated_content_manager_->should_send()) {
    return;
  }

  // Clear previous staged entries.
  std::erase_if(chat_history_, [](const mojom::ConversationTurnPtr& turn) {
    return turn->from_brave_search_SERP;
  });

  // Add the query & summary pairs to the conversation history and notify
  // observers.
  for (const auto& entry : *entries) {
    chat_history_.push_back(mojom::ConversationTurn::New(
        base::Uuid::GenerateRandomV4().AsLowercaseString(),
        CharacterType::HUMAN, mojom::ActionType::QUERY, entry.query,
        std::nullopt /* prompt */, std::nullopt, std::nullopt,
        base::Time::Now(), std::nullopt, std::nullopt, true,
        std::nullopt /* model_key */));
    OnConversationEntryAdded(chat_history_.back());

    std::vector<mojom::ConversationEntryEventPtr> events;
    events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(entry.summary)));
    chat_history_.push_back(mojom::ConversationTurn::New(
        base::Uuid::GenerateRandomV4().AsLowercaseString(),
        CharacterType::ASSISTANT, mojom::ActionType::RESPONSE, entry.summary,
        std::nullopt /* prompt */, std::nullopt, std::move(events),
        base::Time::Now(), std::nullopt, std::nullopt, true,
        std::nullopt /* model_key */));
    OnConversationEntryAdded(chat_history_.back());
  }
}

void ConversationHandler::GeneratePageContent(GetPageContentCallback callback) {
  VLOG(1) << __func__;
  DCHECK(associated_content_manager_->should_send());
  DCHECK(associated_content_manager_->HasAssociatedContent())
      << "Shouldn't have been asked to generate page text when "
      << "|associated_content_manager_->HasContent()| is false.";

  // Make sure user is opted in since this may make a network request
  // for more page content (e.g. video transcript).
  DCHECK(ai_chat_service_->HasUserOptedIn())
      << "UI shouldn't allow operations before user has accepted agreement";
  GeneratePageContentInternal(std::move(callback));
}

void ConversationHandler::GeneratePageContentInternal(
    GetPageContentCallback callback) {
  // Keep hold of the current content so we can check if it changed
  std::string current_content =
      std::string(associated_content_manager_->GetCachedTextContent());
  associated_content_manager_->GetContent(
      base::BindOnce(&ConversationHandler::OnGeneratePageContentComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::move(current_content)));
}

void ConversationHandler::OnGeneratePageContentComplete(
    GetPageContentCallback callback,
    std::string previous_content) {
  auto contents_text = associated_content_manager_->GetCachedTextContent();
  engine_->SanitizeInput(contents_text);

  // Keep is_content_different_ as true if it's the initial state
  is_content_different_ =
      is_content_different_ || contents_text != previous_content;

  std::move(callback).Run(contents_text, associated_content_manager_->IsVideo(),
                          "");

  // Content-used percentage and is_video might have changed in addition to
  // content_type.
  OnAssociatedContentUpdated();
}

void ConversationHandler::OnEngineCompletionDataReceived(
    EngineConsumer::GenerationResultData result) {
  UpdateOrCreateLastAssistantEntry(std::move(result));
}

void ConversationHandler::OnEngineCompletionComplete(
    EngineConsumer::GenerationResult result) {
  is_request_in_progress_ = false;

  if (result.has_value()) {
    DVLOG(2) << __func__ << ": With value";
    // Handle success, which might mean do nothing much since all
    // data was passed in the streaming "received" callback.
    if (result->event && result->event->is_completion_event() &&
        !result->event->get_completion_event()->completion.empty()) {
      UpdateOrCreateLastAssistantEntry(std::move(*result));
      OnConversationEntryAdded(chat_history_.back());
    } else {
      auto& last_entry = chat_history_.back();
      if (last_entry->character_type != mojom::CharacterType::ASSISTANT) {
        SetAPIError(mojom::APIError::ConnectionIssue);
      } else {
        OnConversationEntryAdded(chat_history_.back());
      }
    }
    MaybePopPendingRequests();
  } else {
    // handle failure
    if (result.error() != mojom::APIError::None) {
      DVLOG(2) << __func__ << ": With error";
      SetAPIError(std::move(result.error()));
    } else {
      DVLOG(2) << __func__ << ": With no error";
      // No error but check if no content was received
      auto& last_entry = chat_history_.back();
      if (last_entry->character_type != mojom::CharacterType::ASSISTANT) {
        SetAPIError(mojom::APIError::ConnectionIssue);
      } else {
        SetAPIError(mojom::APIError::None);
      }
    }
  }

  OnAPIRequestInProgressChanged();
}

void ConversationHandler::OnSuggestedQuestionsResponse(
    EngineConsumer::SuggestedQuestionResult result) {
  if (result.has_value()) {
    std::ranges::transform(result.value(), std::back_inserter(suggestions_),
                           [](const auto& s) { return Suggestion(s); });
    suggestion_generation_status_ =
        mojom::SuggestionGenerationStatus::HasGenerated;
    DVLOG(2) << "Got questions:" << base::JoinString(result.value(), "\n");
  } else {
    // TODO(nullhook): Set a specialized error state generated questions
    suggestion_generation_status_ =
        mojom::SuggestionGenerationStatus::CanGenerate;
    DVLOG(2) << "Got no questions";
  }

  // Notify observers
  OnSuggestedQuestionsChanged();
}

void ConversationHandler::OnModelListUpdated() {
  OnModelDataChanged();

  const mojom::Model* model = model_service_->GetModel(model_key_);

  if (model && engine_) {
    engine_->UpdateModelOptions(*model->options);
  }
}

void ConversationHandler::OnDefaultModelChanged(const std::string& old_key,
                                                const std::string& new_key) {
  // When default model changes, change any conversation that
  // has that model.
  DVLOG(1) << "Default model changed from " << old_key << " to " << new_key;
  if (model_key_ == old_key) {
    ChangeModel(new_key);
  }
}

void ConversationHandler::OnModelRemoved(const std::string& removed_key) {
  // Any current model is removed, switch to default
  if (model_key_ == removed_key) {
    // TODO(nullhook): Inform the UI that the model has been removed, so it can
    // show a message
    model_key_ = model_service_->GetDefaultModelKey();
  }

  // Update the engine and fetch the new models
  InitEngine();
}

void ConversationHandler::OnModelDataChanged() {
  const std::vector<mojom::ModelPtr>& models = model_service_->GetModels();

  for (const mojo::Remote<mojom::ConversationUI>& client :
       conversation_ui_handlers_) {
    std::vector<mojom::ModelPtr> models_copy(models.size());
    std::transform(models.cbegin(), models.cend(), models_copy.begin(),
                   [](auto& model) { return model.Clone(); });
    client->OnModelDataChanged(model_key_, std::move(models_copy));
  }
  OnStateForConversationEntriesChanged();
}

void ConversationHandler::OnHistoryUpdate(mojom::ConversationTurnPtr entry) {
  // TODO(petemill): Provide the updated converation history item so that
  // we don't need to clone every entry.
  for (auto& client : conversation_ui_handlers_) {
    client->OnConversationHistoryUpdate(entry ? entry.Clone() : nullptr);
  }
  for (auto& client : untrusted_conversation_ui_handlers_) {
    client->OnConversationHistoryUpdate(entry ? entry.Clone() : nullptr);
  }
}

void ConversationHandler::OnConversationEntryRemoved(
    std::optional<std::string> entry_uuid) {
  OnHistoryUpdate(nullptr);
  if (!entry_uuid.has_value()) {
    return;
  }
  for (auto& observer : observers_) {
    observer.OnConversationEntryRemoved(this, entry_uuid.value());
  }
}

void ConversationHandler::OnConversationEntryAdded(
    mojom::ConversationTurnPtr& entry) {
  // Only notify about staged entries once we have the first staged entry
  if (entry->from_brave_search_SERP) {
    OnHistoryUpdate(nullptr);
    return;
  }
  std::optional<std::vector<std::string_view>> associated_content_value;
  if (is_content_different_ &&
      associated_content_manager_->HasAssociatedContent()) {
    associated_content_value = associated_content_manager_->GetCachedContent();
    is_content_different_ = false;
  }
  // If this is the first entry that isn't staged, notify about all previous
  // staged entries
  if (!entry->from_brave_search_SERP &&
      std::ranges::all_of(chat_history_,
                          [&entry](mojom::ConversationTurnPtr& history_entry) {
                            return history_entry == entry ||
                                   history_entry->from_brave_search_SERP;
                          })) {
    // Notify every item in chat history
    for (auto& observer : observers_) {
      for (auto& history_entry : chat_history_) {
        observer.OnConversationEntryAdded(this, history_entry,
                                          associated_content_value);
      }
    }
    OnHistoryUpdate(nullptr);
    return;
  }
  for (auto& observer : observers_) {
    observer.OnConversationEntryAdded(this, entry, associated_content_value);
  }
  OnHistoryUpdate(entry.Clone());
}

void ConversationHandler::UpdateAssociatedContentInfo() {
  metadata_->associated_content =
      associated_content_manager_->GetAssociatedContent();
}

mojom::ConversationEntriesStatePtr
ConversationHandler::GetStateForConversationEntries() {
  auto& model = GetCurrentModel();
  bool is_leo_model = model.options->is_leo_model_options();

  const auto& models = model_service_->GetModels();
  std::vector<mojom::ModelPtr> models_copy(models.size());
  std::transform(models.cbegin(), models.cend(), models_copy.begin(),
                 [](auto& model) { return model.Clone(); });

  mojom::ConversationEntriesStatePtr entries_state =
      mojom::ConversationEntriesState::New();
  entries_state->is_generating = IsRequestInProgress();
  entries_state->is_leo_model = is_leo_model;
  entries_state->all_models = std::move(models_copy);
  entries_state->current_model_key = model.key;
  entries_state->total_tokens = metadata_->total_tokens;
  entries_state->trimmed_tokens = metadata_->trimmed_tokens;
  entries_state->content_used_percentage =
      !metadata_->associated_content.empty()
          ? std::make_optional(
                metadata_->associated_content[0]->content_used_percentage)
          : std::nullopt;
  // Can't submit if not a premium user and the model is premium-only
  entries_state->can_submit_user_entries =
      !IsRequestInProgress() &&
      (ai_chat_service_->IsPremiumStatus() || !is_leo_model ||
       model.options->get_leo_model_options()->access !=
           mojom::ModelAccess::PREMIUM);
  return entries_state;
}

void ConversationHandler::OnClientConnectionChanged() {
  DVLOG(2) << metadata_->uuid << " has " << receivers_.size()
           << " RECEIVERS and " << conversation_ui_handlers_.size()
           << " UI HANDLERS";
  for (auto& observer : observers_) {
    observer.OnClientConnectionChanged(this);
  }
}

void ConversationHandler::OnConversationTitleChanged(std::string_view title) {
  for (auto& observer : observers_) {
    observer.OnConversationTitleChanged(metadata_->uuid, std::string(title));
  }
}

void ConversationHandler::OnConversationTokenInfoChanged(
    uint64_t total_tokens,
    uint64_t trimmed_tokens) {
  for (auto& observer : observers_) {
    observer.OnConversationTokenInfoChanged(metadata_->uuid, total_tokens,
                                            trimmed_tokens);
  }
}

void ConversationHandler::OnConversationUIConnectionChanged(
    mojo::RemoteSetElementId id) {
  OnClientConnectionChanged();
}

void ConversationHandler::OnSelectedLanguageChanged(
    const std::string& selected_language) {
  selected_language_ = selected_language;
}

void ConversationHandler::OnSuggestedQuestionsChanged() {
  std::vector<std::string> suggestions;
  std::ranges::transform(suggestions_, std::back_inserter(suggestions),
                         [](const auto& s) { return s.title; });

  for (auto& client : conversation_ui_handlers_) {
    client->OnSuggestedQuestionsChanged(suggestions,
                                        suggestion_generation_status_);
  }
}

void ConversationHandler::OnAPIRequestInProgressChanged() {
  OnStateForConversationEntriesChanged();
  for (auto& client : conversation_ui_handlers_) {
    client->OnAPIRequestInProgress(is_request_in_progress_);
  }
  for (auto& observer : observers_) {
    observer.OnRequestInProgressChanged(this, is_request_in_progress_);
  }
}

void ConversationHandler::OnStateForConversationEntriesChanged() {
  auto entries_state = GetStateForConversationEntries();
  for (auto& client : untrusted_conversation_ui_handlers_) {
    client->OnEntriesUIStateChanged(entries_state->Clone());
  }
}

size_t ConversationHandler::GetConversationHistorySize() {
  return GetConversationHistory().size();
}

void ConversationHandler::GetScreenshots(GetScreenshotsCallback callback) {
  if (associated_content_manager_->HasAssociatedContent()) {
    associated_content_manager_->GetScreenshots(std::move(callback));
  } else {
    std::move(callback).Run(std::nullopt);
  }
}

bool ConversationHandler::should_send_page_contents() const {
  return associated_content_manager_->should_send();
}

mojom::APIError ConversationHandler::current_error() const {
  return current_error_;
}

void ConversationHandler::SetTemporary(bool temporary) {
  // This API is limited to the start of the conversation.
  if (metadata_->has_content) {
    return;
  }

  metadata_->temporary = temporary;
}

}  // namespace ai_chat

#undef STARTER_PROMPT
