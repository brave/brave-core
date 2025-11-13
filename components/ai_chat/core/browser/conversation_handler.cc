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
#include <variant>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/span.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
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
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/model_validator.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
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
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::vector<std::unique_ptr<ToolProvider>> tool_providers)
    : ConversationHandler(conversation,
                          ai_chat_service,
                          model_service,
                          credential_manager,
                          feedback_api,
                          prefs,
                          url_loader_factory,
                          std::move(tool_providers),
                          std::nullopt) {}

ConversationHandler::ConversationHandler(
    mojom::Conversation* conversation,
    AIChatService* ai_chat_service,
    ModelService* model_service,
    AIChatCredentialManager* credential_manager,
    AIChatFeedbackAPI* feedback_api,
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::vector<std::unique_ptr<ToolProvider>> tool_providers,
    std::optional<mojom::ConversationArchivePtr> initial_state)
    : associated_content_manager_(
          std::make_unique<AssociatedContentManager>(this)),
      tool_providers_(std::move(tool_providers)),
      metadata_(conversation),
      ai_chat_service_(ai_chat_service),
      model_service_(model_service),
      credential_manager_(credential_manager),
      feedback_api_(feedback_api),
      prefs_(prefs),
      url_loader_factory_(url_loader_factory) {
  // Set conversation capability based on profile-global state.
  // TODO(https://github.com/brave/brave-browser/issues/49261): This is
  // temporary whilst content agent conversations are
  // 1) not toggleable by the user and
  // 2) only for specific profiles.
  // When this is toggleable by the user,
  // we should have some client function that changes the conversation
  // capability. And when this is not global to a Profile, we should not have
  // the service make the determination.
  if (ai_chat_service_->GetIsContentAgentAllowed()) {
    conversation_capability_ = mojom::ConversationCapability::CONTENT_AGENT;
  }

  // Observe tool providers
  for (const auto& tool_provider : tool_providers_) {
    tool_provider->AddObserver(this);
  }

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
  for (const auto& tool_provider : tool_providers_) {
    tool_provider->RemoveObserver(this);
  }
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

  untrusted_conversation_ui_handlers_.Get(id)->AssociatedContentChanged(
      associated_content_manager_->GetAssociatedContent());
}

void ConversationHandler::OnArchiveContentUpdated(
    mojom::ConversationArchivePtr conversation_data) {
  associated_content_manager_->LoadArchivedContent(metadata_,
                                                   conversation_data);
}

void ConversationHandler::OnAssociatedContentUpdated() {
  metadata_->associated_content =
      associated_content_manager_->GetAssociatedContent();
  auto& associated_content = metadata_->associated_content;

  // Clone the content to avoid multiple calls to GetAssociatedContent.
  auto clone_content = [&associated_content]() {
    std::vector<mojom::AssociatedContentPtr> cloned_content;
    std::ranges::transform(
        associated_content, std::back_inserter(cloned_content),
        [](const auto& content) { return content->Clone(); });
    return cloned_content;
  };

  for (auto& client : conversation_ui_handlers_) {
    client->OnAssociatedContentInfoChanged(clone_content());
  }

  for (const auto& client : untrusted_conversation_ui_handlers_) {
    client->AssociatedContentChanged(clone_content());
  }

  OnStateForConversationEntriesChanged();
  MaybeSeedOrClearSuggestions();
  MaybeFetchOrClearContentStagedConversation();

  for (auto& observer : observers_) {
    observer.OnAssociatedContentUpdated(this);
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
      !associated_content_manager_->GetCachedContents().empty()) {
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
  auto default_model_key = model_service_->GetDefaultModelKey();

  std::vector<std::string> suggestions;
  std::ranges::transform(suggestions_, std::back_inserter(suggestions),
                         [](const auto& s) { return s.title; });

  mojom::ConversationStatePtr state = mojom::ConversationState::New(
      metadata_->uuid, is_request_in_progress_, std::move(models_copy),
      model_key, default_model_key, std::move(suggestions),
      suggestion_generation_status_,
      associated_content_manager_->GetAssociatedContent(), current_error_,
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

  // Auto-switch to vision model if needed
  MaybeSwitchToVisionModel(uploaded_files);

  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      std::nullopt, CharacterType::HUMAN, mojom::ActionType::QUERY, input,
      std::nullopt /* prompt */, std::nullopt /* selected_text */,
      std::nullopt /* events */, base::Time::Now(), std::nullopt /* edits */,
      std::move(uploaded_files), nullptr /* skill */, false,
      std::nullopt /* model_key */);
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

  // Submitting a new human entry takes precedence over any pending tool
  // requests, so if we have a previous assistant entry, cancel any pending tool
  // requests.
  if (!chat_history_.empty()) {
    auto& last_entry = chat_history_.back();
    if (last_entry->character_type == mojom::CharacterType::ASSISTANT &&
        last_entry->events && !last_entry->events->empty()) {
      // Delete any event that is_tool_use_event and has no output
      std::erase_if(last_entry->events.value(), [](const auto& event) {
        return event->is_tool_use_event() &&
               !event->get_tool_use_event()->output.has_value();
      });
    }
  }

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
  // Give tools a chance to reset their state for the next loop
  InitToolsForNewGenerationLoop();
  // Get any content and perform the response generation
  PerformAssistantGenerationWithPossibleContent();
}

void ConversationHandler::SubmitHumanConversationEntryWithAction(
    const std::string& input,
    mojom::ActionType action_type) {
  DCHECK(!is_request_in_progress_)
      << "Should not be able to submit more"
      << "than a single human conversation turn at a time.";

  SubmitSelectedText(input, action_type);
}

void ConversationHandler::SubmitHumanConversationEntryWithSkill(
    const std::string& input,
    const std::string& skill_id) {
  DCHECK(!is_request_in_progress_)
      << "Should not be able to submit more"
      << "than a single human conversation turn at a time.";

  // Get Skill from prefs and convert to SkillEntry object
  auto skill = prefs::GetSkillFromPrefs(*prefs_, skill_id);

  mojom::SkillEntryPtr skill_entry = nullptr;

  if (skill) {
    // Switch model if specified and different from current model
    if (skill->model && !skill->model->empty() &&
        *skill->model != GetCurrentModel().key) {
      ChangeModel(*skill->model);
    }

    // Create Skill entry
    skill_entry = mojom::SkillEntry::New(skill->shortcut, skill->prompt);

    // Update last_used time
    prefs::UpdateSkillLastUsedInPrefs(skill_id, *prefs_);
  }

  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      std::nullopt, CharacterType::HUMAN, mojom::ActionType::QUERY, input,
      std::nullopt /* prompt */, std::nullopt /* selected_text */,
      std::nullopt /* events */, base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_files */, std::move(skill_entry), false,
      std::nullopt /* model_key */);

  SubmitHumanConversationEntry(std::move(turn));
}

void ConversationHandler::ModifyConversation(const std::string& entry_uuid,
                                             const std::string& new_text) {
  CHECK(!entry_uuid.empty()) << "Entry UUID is empty";
  auto turn_it = std::ranges::find_if(
      chat_history_,
      [&entry_uuid](const auto& turn) { return turn->uuid == entry_uuid; });
  if (turn_it == chat_history_.end()) {
    return;
  }

  auto& turn = *turn_it;
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
        std::nullopt, nullptr /* skill */, false, turn->model_key);
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
      base::Time::Now(), std::nullopt /* edits */, std::nullopt,
      nullptr /* skill */, false, turn->model_key);
  if (!turn->edits) {
    turn->edits.emplace();
  }
  // Erase all turns after the edited turn (turn_it) and notify observers
  auto turn_index = std::distance(chat_history_.begin(), turn_it);
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

  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      std::nullopt, CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_PAGE,
      l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE),
      l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE),
      std::nullopt /* selected_text */, std::nullopt /* events */,
      base::Time::Now(), std::nullopt /* edits */,
      std::nullopt /* uploaded_images */, nullptr /* skill */, false,
      std::nullopt /* model_key */);
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
      std::nullopt, nullptr /* skill */, false, std::nullopt /* model_key */);
  SubmitHumanConversationEntry(std::move(turn));

  // Remove the suggestion from the list, assume the list has been modified
  // inside SubmitHumanConversationEntry so search for it again.
  auto to_remove =
      std::ranges::remove(suggestions_, suggestion_title, &Suggestion::title);
  suggestions_.erase(to_remove.begin(), to_remove.end());
  OnSuggestedQuestionsChanged();
}

const std::vector<ConversationHandler::Suggestion>&
ConversationHandler::GetSuggestedQuestionsForTest() const {
  return suggestions_;
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
      associated_content_manager_->HasAssociatedContent() ? 1u : 0u;
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

void ConversationHandler::PerformQuestionGeneration() {
  engine_->GenerateQuestionSuggestions(
      associated_content_manager_->GetCachedContents(), selected_language_,
      base::BindOnce(&ConversationHandler::OnSuggestedQuestionsResponse,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ConversationHandler::GetAssociatedContentInfo(
    GetAssociatedContentInfoCallback callback) {
  std::move(callback).Run(associated_content_manager_->GetAssociatedContent());
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
      std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt /* model_key */);

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
    associated_content_manager_->ClearContent();
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
      std::nullopt, std::nullopt, nullptr /* skill */, false,
      std::nullopt /* model_key */);
  AddToConversationHistory(std::move(turn));
  SetAPIError(error);
}

void ConversationHandler::OnUserOptedIn() {
  MaybePopPendingRequests();
  MaybeFetchOrClearContentStagedConversation();
}

void ConversationHandler::RespondToToolUseRequest(
    const std::string& tool_use_id,
    std::vector<mojom::ContentBlockPtr> output) {
  auto* tool_use = GetToolUseEventForLastResponse(tool_use_id);
  if (!tool_use) {
    DLOG(ERROR) << "Tool use event not found: " << tool_use_id;
    is_tool_use_in_progress_ = false;
    OnAPIRequestInProgressChanged();
    return;
  }

  // Tool uses require output. Whilst we could have some tools
  // produce no output in order to prevent wasted API calls
  // for user-only actions, it could lead to weird behavior when we have
  // to remove the tool use event from the history before we send to the API
  // (since the APIs don't like tool use requests with no output).
  CHECK(!output.empty()) << "No output from tool use: " << tool_use->tool_name
                         << " (" << tool_use_id << ")";

  DVLOG(0) << "got output for tool: " << tool_use->tool_name;

  tool_use->output = std::move(output);

  OnToolUseEventOutput(chat_history_.back().get(), tool_use);

  // Only perform generation if there are no pending tools left to run from
  // the last entry.
  if (std::ranges::all_of(
          *chat_history_.back()->events,
          [](const mojom::ConversationEntryEventPtr& event) {
            return !event->is_tool_use_event() ||
                   event->get_tool_use_event()->output.has_value();
          })) {
    DVLOG(0) << "No more tool use requests to handle, performing generation";
    is_request_in_progress_ = true;
    is_tool_use_in_progress_ = false;
    OnAPIRequestInProgressChanged();
    PerformAssistantGenerationWithPossibleContent();
  } else {
    DVLOG(0) << "Tool use request handled, but still have more to handle";
    // Still have more tool use requests to handle.
    MaybeRespondToNextToolUseRequest();
  }
}

void ConversationHandler::ProcessPermissionChallenge(
    const std::string& tool_use_id,
    bool user_result) {
  auto* tool_use = GetToolUseEventForLastResponse(tool_use_id);
  if (!tool_use) {
    DLOG(ERROR) << "Tool use event not found: " << tool_use_id;
    return;
  }

  if (!tool_use->permission_challenge) {
    DLOG(ERROR) << "No permission challenge for tool use: " << tool_use_id;
    return;
  }

  DVLOG(0) << __func__ << " user " << (user_result ? "approved" : "denied")
           << " permission for: " << tool_use->tool_name;

  if (!user_result) {
    // User declined - send rejection output and stop tool loop
    std::vector<mojom::ContentBlockPtr> result;
    result.push_back(mojom::ContentBlock::NewTextContentBlock(
        mojom::TextContentBlock::New("Permission to use this tool with these "
                                     "arguments was denied by the user.")));

    // Set output and notify UI
    tool_use->output = std::move(result);
    OnToolUseEventOutput(chat_history_.back().get(), tool_use);

    // Directly call generation, bypassing the normal tool loop continuation
    // This stops processing of any remaining tools in this turn
    DVLOG(0)
        << "Permission denied, stopping tool loop and performing generation";
    is_request_in_progress_ = true;
    is_tool_use_in_progress_ = false;
    OnAPIRequestInProgressChanged();
    PerformAssistantGenerationWithPossibleContent();
    return;
  }

  // User approved - clear the permission challenge
  tool_use->permission_challenge = nullptr;

  // Notify UI of the state change
  OnToolUseEventOutput(chat_history_.back().get(), tool_use);

  // Find the tool and notify it
  base::WeakPtr<Tool> tool_ptr;
  for (auto& tool : GetTools()) {
    if (tool && tool->Name() == tool_use->tool_name) {
      tool_ptr = tool;
      break;
    }
  }

  if (!tool_ptr) {
    DLOG(ERROR) << "Tool not found: " << tool_use->tool_name;
    return;
  }

  // Notify tool that permission was granted. At the moment there is no
  // need to distinguish between different permission challenges. If that
  // changes then we can add relevant parameters to this method.
  tool_ptr->UserPermissionGranted(tool_use_id);

  // Continue with tool execution
  MaybeRespondToNextToolUseRequest();
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

void ConversationHandler::InitToolsForNewGenerationLoop() {
  // We can reset any already-created tools that don't want state to
  // survive between loops (i.e. when a new user message is received).
  for (auto& tool_provider : tool_providers_) {
    tool_provider->OnNewGenerationLoop();
  }
}

void ConversationHandler::PerformAssistantGenerationWithPossibleContent() {
  if (associated_content_manager_->HasAssociatedContent()) {
    // Fetch updated page content before performing generation
    GeneratePageContent(
        base::BindOnce(&ConversationHandler::PerformAssistantGeneration,
                       weak_ptr_factory_.GetWeakPtr()));
  } else {
    // Now the conversation is committed, we can remove some unneccessary data
    // if we're not associated with a page.
    suggestions_.clear();

    // Reset the content to be empty.
    associated_content_manager_->ClearContent();

    OnSuggestedQuestionsChanged();
    // Perform generation immediately
    PerformAssistantGeneration();
  }
}

void ConversationHandler::PerformAssistantGeneration() {
  if (chat_history_.empty()) {
    DLOG(ERROR) << "Cannot generate assistant response without any history";
    return;
  }

  // When the next response is generated, create a new entry instead of
  // appending to the previous so that we know which tool use requests should be
  // dealt with. When responding to a tool use request, we end up with 2
  // assistant entries in a row.
  needs_new_entry_ = true;

  engine_->GenerateAssistantResponse(
      associated_content_manager_->GetCachedContentsMap(), chat_history_,
      selected_language_, IsTemporaryChat(), GetTools(),
      std::nullopt /* preferred_tool_name */, conversation_capability_,
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
  if (needs_new_entry_ || chat_history_.empty() ||
      chat_history_.back()->character_type != CharacterType::ASSISTANT) {
    needs_new_entry_ = false;
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New(
        base::Uuid::GenerateRandomV4().AsLowercaseString(),
        CharacterType::ASSISTANT, mojom::ActionType::RESPONSE, "",
        std::nullopt /* prompt */, std::nullopt,
        std::vector<mojom::ConversationEntryEventPtr>{}, base::Time::Now(),
        std::nullopt, std::nullopt, nullptr /* skill */, false,
        result.model_key);
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

  if (event->is_tool_use_event() && entry->events->size() > 0) {
    // Tool use events can be partial and may need to be combined with the
    // previous event.
    auto& last_event = entry->events->back();
    auto& tool_use_event = event->get_tool_use_event();

    DVLOG(2) << __func__
             << " Got event for tool use: " << tool_use_event->tool_name
             << " is empty? " << tool_use_event->tool_name.empty()
             << " with input: " << tool_use_event->arguments_json;

    if (last_event->is_tool_use_event() && tool_use_event->tool_name.empty()) {
      last_event->get_tool_use_event()->arguments_json =
          base::StrCat({last_event->get_tool_use_event()->arguments_json,
                        tool_use_event->arguments_json});
      // TODO(petemill): Don't clone
      OnHistoryUpdate(entry.Clone());
      return;
    }
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
  if (!associated_content_manager_->HasAssociatedContent()) {
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
  } else if (!suggestions_.empty() &&
             (suggestions_[0].action_type ==
                  mojom::ActionType::SUMMARIZE_PAGE ||
              suggestions_[0].action_type ==
                  mojom::ActionType::SUMMARIZE_VIDEO)) {
    // Update the title for the summarize page/video suggestion. Note: We always
    // treat multiple associated content items as a page for now.
    suggestions_[0].title =
        associated_content_manager_->IsVideo()
            ? l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_VIDEO)
            : l10n_util::GetPluralStringFUTF8(
                  IDS_CHAT_UI_SUMMARIZE_PAGES_SUGGESTION,
                  associated_content_manager_->GetContentDelegateCount());
    suggestions_[0].prompt =
        associated_content_manager_->IsVideo()
            ? l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_VIDEO)
            : l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE);
    suggestions_[0].action_type = associated_content_manager_->IsVideo()
                                      ? mojom::ActionType::SUMMARIZE_VIDEO
                                      : mojom::ActionType::SUMMARIZE_PAGE;
  }
  OnSuggestedQuestionsChanged();
}

void ConversationHandler::MaybeFetchOrClearContentStagedConversation() {
  // Try later when we get a connected client
  if (!IsAnyClientConnected()) {
    return;
  }

  if (!associated_content_manager_->HasAssociatedContent()) {
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
      !associated_content_manager_->HasAssociatedContent()) {
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
        base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */,
        true, std::nullopt /* model_key */));
    OnConversationEntryAdded(chat_history_.back());

    std::vector<mojom::ConversationEntryEventPtr> events;
    events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(entry.summary, std::nullopt)));
    chat_history_.push_back(mojom::ConversationTurn::New(
        base::Uuid::GenerateRandomV4().AsLowercaseString(),
        CharacterType::ASSISTANT, mojom::ActionType::RESPONSE, entry.summary,
        std::nullopt /* prompt */, std::nullopt, std::move(events),
        base::Time::Now(), std::nullopt, std::nullopt, nullptr /* skill */,
        true, std::nullopt /* model_key */));
    OnConversationEntryAdded(chat_history_.back());
  }
}

void ConversationHandler::GeneratePageContent(base::OnceClosure callback) {
  VLOG(1) << __func__;
  CHECK(associated_content_manager_->HasAssociatedContent())
      << "Shouldn't have been asked to generate page text when "
      << "|associated_content_manager_->HasContent()| is false.";

  // Make sure user is opted in since this may make a network request
  // for more page content (e.g. video transcript).
  DCHECK(ai_chat_service_->HasUserOptedIn())
      << "UI shouldn't allow operations before user has accepted agreement";
  GeneratePageContentInternal(std::move(callback));
}

void ConversationHandler::GeneratePageContentInternal(
    base::OnceClosure callback) {
  associated_content_manager_->HasContentUpdated(
      base::BindOnce(&ConversationHandler::OnGeneratePageContentComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ConversationHandler::OnGeneratePageContentComplete(
    base::OnceClosure callback,
    bool content_changed) {
  // Keep is_content_different_ as true if it's the initial state
  is_content_different_ = is_content_different_ || content_changed;

  // Check if content is empty and take screenshots if needed
  if (std::ranges::all_of(associated_content_manager_->GetCachedContents(),
                          [](const auto& page_content_ref) {
                            auto page_content = page_content_ref.get();
                            base::TrimWhitespaceASCII(page_content.content,
                                                      base::TRIM_ALL,
                                                      &page_content.content);
                            return page_content.content.empty();
                          }) &&
      associated_content_manager_->HasAssociatedContent()) {
    // Check if the conversation already has screenshots
    bool has_screenshots = std::ranges::any_of(
        chat_history_, [](const mojom::ConversationTurnPtr& turn) {
          return turn->uploaded_files &&
                 std::ranges::any_of(
                     *turn->uploaded_files,
                     [](const mojom::UploadedFilePtr& file) {
                       return file->type ==
                              mojom::UploadedFileType::kScreenshot;
                     });
        });

    // Only take screenshots if no screenshots have been taken yet
    if (!has_screenshots) {
      associated_content_manager_->GetScreenshots(
          base::BindOnce(&ConversationHandler::OnAutoScreenshotsTaken,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
      return;
    }
  }

  std::move(callback).Run();

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
  // Handle failure
  if (!result.has_value()) {
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
    CompleteGeneration(false);
    return;
  }

  // Handle success, which might mean do nothing much since all data was passed
  // in the streaming "received" callback.
  DVLOG(2) << __func__ << ": With value";
  if (result->event && result->event->is_completion_event() &&
      !result->event->get_completion_event()->completion.empty()) {
    UpdateOrCreateLastAssistantEntry(std::move(*result));
  } else {
    // This is a workaround for any occasions where the engine returns
    // a success but there was no new entry.
    if (needs_new_entry_) {
      SetAPIError(mojom::APIError::ConnectionIssue);
      CompleteGeneration(false);
      return;
    }
  }
  OnConversationEntryAdded(chat_history_.back());

  // Check if we need title generation (after assistant response is added)
  if (engine_->RequiresClientSideTitleGeneration() &&
      chat_history_.size() == 2) {
    // Keep the request active and complete the generation in OnTitleGenerated.
    engine_->GenerateConversationTitle(
        associated_content_manager_->GetCachedContentsMap(), chat_history_,
        base::BindOnce(&ConversationHandler::OnTitleGenerated,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  // Complete the generation if we don't need title generation.
  CompleteGeneration(true);
}

void ConversationHandler::OnTitleGenerated(
    EngineConsumer::GenerationResult result) {
  // Process successful title result and ignore title errors silently.
  if (result.has_value() && result->event &&
      result->event->is_conversation_title_event()) {
    OnConversationTitleChanged(
        result->event->get_conversation_title_event()->title);
  }

  CompleteGeneration(true);
}

void ConversationHandler::CompleteGeneration(bool success) {
  is_request_in_progress_ = false;
  if (success) {
    MaybePopPendingRequests();
  }
  MaybeRespondToNextToolUseRequest();
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
  } else {
    // If this conversation is not using the old default model, we still need to
    // notify the UI about the default model change without changing the current
    // model. This ensures the UI gets updated with the new default model
    // information even when the conversation is using a different model.
    OnModelDataChanged();
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

void ConversationHandler::OnContentTaskStarted(int32_t tab_id) {
  // Store the tab_id so consumers can validate a tab is used by a tool
  // in this conversation, or that a tab can locate its controlling
  // conversation.
  task_tab_ids_.insert(tab_id);

  // Notify clients so they may display the relationship in UI
  for (auto& client : untrusted_conversation_ui_handlers_) {
    client->ContentTaskStarted(tab_id);
  }
}

void ConversationHandler::OnModelDataChanged() {
  const std::vector<mojom::ModelPtr>& models = model_service_->GetModels();
  auto default_model_key = model_service_->GetDefaultModelKey();

  for (const mojo::Remote<mojom::ConversationUI>& client :
       conversation_ui_handlers_) {
    std::vector<mojom::ModelPtr> models_copy(models.size());
    std::transform(models.cbegin(), models.cend(), models_copy.begin(),
                   [](auto& model) { return model.Clone(); });
    client->OnModelDataChanged(model_key_, default_model_key,
                               std::move(models_copy));
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

void ConversationHandler::OnToolUseEventOutput(mojom::ConversationTurn* entry,
                                               mojom::ToolUseEvent* tool_use) {
  CHECK(entry->uuid.has_value()) << "Cannot update tool use event for entry "
                                    "without uuid";
  for (auto& client : untrusted_conversation_ui_handlers_) {
    client->OnToolUseEventOutput(entry->uuid.value(), tool_use->Clone());
  }

  CHECK(entry->events.has_value()) << "Cannot update tool use event for entry "
                                      "without events";
  auto event_order =
      std::ranges::find_if(entry->events.value(),
                           [tool_use](const auto& event) {
                             return event->is_tool_use_event() &&
                                    event->get_tool_use_event()->id ==
                                        tool_use->id;
                           }) -
      entry->events->begin();
  for (auto& observer : observers_) {
    observer.OnToolUseEventOutput(this, entry->uuid.value(), event_order,
                                  tool_use->Clone());
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
  if (entry->character_type == mojom::CharacterType::HUMAN) {
    associated_content_manager_->AssociateUnsentContentWithTurn(entry);
  }

  // Only notify about staged entries once we have the first staged entry
  if (entry->from_brave_search_SERP) {
    OnHistoryUpdate(nullptr);
    return;
  }
  std::optional<PageContents> associated_content_value;
  if (is_content_different_ &&
      associated_content_manager_->HasAssociatedContent()) {
    associated_content_value = associated_content_manager_->GetCachedContents();
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

  entries_state->is_generating =
      IsRequestInProgress() || is_tool_use_in_progress_;
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
  entries_state->visual_content_used_percentage =
      visual_content_used_percentage_;
  // Can't submit if not a premium user and the model is premium-only
  entries_state->can_submit_user_entries =
      !IsRequestInProgress() &&
      (ai_chat_service_->IsPremiumStatus() || !is_leo_model ||
       model.options->get_leo_model_options()->access !=
           mojom::ModelAccess::PREMIUM);
  entries_state->conversation_capability = conversation_capability_;
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
    client->OnAPIRequestInProgress(is_request_in_progress_ ||
                                   is_tool_use_in_progress_);
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

std::vector<base::WeakPtr<Tool>> ConversationHandler::GetTools() {
  std::vector<base::WeakPtr<Tool>> tools;
  // Get provided tools
  for (auto& tool_provider : tool_providers_) {
    std::ranges::move(tool_provider->GetTools(), std::back_inserter(tools));
  }

  // Filter tools not supported
  auto& model = GetCurrentModel();
  tools.erase(
      std::remove_if(
          tools.begin(), tools.end(),
          [&](auto& tool) {
            return (!tool->IsSupportedByModel(model) ||
                    !tool->SupportsConversation(
                        GetIsTemporary(),
                        associated_content_manager_->HasAssociatedContent(),
                        conversation_capability_));
          }),
      tools.end());

  return tools;
}

mojom::ToolUseEvent* ConversationHandler::GetToolUseEventForLastResponse(
    std::string_view tool_id) {
  if (!chat_history_.empty()) {
    auto& last_entry = chat_history_.back();
    if (last_entry->character_type == mojom::CharacterType::ASSISTANT &&
        last_entry->events->size() > 0) {
      for (auto& event : *last_entry->events) {
        if (event->is_tool_use_event()) {
          auto& tool_use_event = event->get_tool_use_event();
          if (tool_use_event->id != tool_id) {
            continue;
          }
          return tool_use_event.get();
        }
      }
    }
  }
  return nullptr;
}

void ConversationHandler::MaybeRespondToNextToolUseRequest() {
  // Continue the loop of tool use handling and completion continuing until
  // either:
  // - A response comes back with no tool use requests
  // - Any tool use requests require user interaction (permission challenge
  //   or providing output via UI)

  is_tool_use_in_progress_ = false;
  OnAPIRequestInProgressChanged();

  if (chat_history_.empty()) {
    return;
  }
  auto& last_entry = chat_history_.back();
  if (last_entry->character_type != mojom::CharacterType::ASSISTANT ||
      last_entry->events->size() == 0) {
    return;
  }

  // Handle one tool at a time and wait for its response
  // before handling the next one
  for (auto& event : *last_entry->events) {
    if (event->is_tool_use_event()) {
      auto& tool_use_event = event->get_tool_use_event();
      if (tool_use_event->output != std::nullopt) {
        // already handled
        continue;
      }

      // Check for existing permission challenge that hasn't been
      // granted yet. If permission_challenge exists, permission is needed.
      if (tool_use_event->permission_challenge) {
        DVLOG(1) << __func__ << " tool waiting for permission: "
                 << tool_use_event->tool_name;
        OnToolUseEventOutput(last_entry.get(), tool_use_event.get());
        break;
      }

      // Find the tool
      base::WeakPtr<Tool> tool_ptr;
      for (auto& tool : GetTools()) {
        if (!tool) {
          // Defensive check to avoid dereferencing invalid WeakPtr
          DVLOG(1) << "Skipping invalid tool weak_ptr during execution";
          continue;
        }

        if (!tool_use_event->tool_name.empty() &&
            tool->Name() == tool_use_event->tool_name) {
          tool_ptr = tool;
          break;
        }
      }

      if (!tool_ptr) {
        DVLOG(1) << "Tool not found or unavailable: "
                 << tool_use_event->tool_name;

        is_tool_use_in_progress_ = true;
        OnAPIRequestInProgressChanged();

        std::vector<mojom::ContentBlockPtr> result;
        result.push_back(mojom::ContentBlock::NewTextContentBlock(
            mojom::TextContentBlock::New(
                base::StrCat({"The ", tool_use_event->tool_name,
                              " tool is not available."}))));

        RespondToToolUseRequest(tool_use_event->id, std::move(result));
        break;
      }

      // Check if tool requires user interaction.
      // Only check if there isn't already a pending permission_challenge.
      if (!tool_use_event->permission_challenge) {
        auto interaction_result =
            tool_ptr->RequiresUserInteractionBeforeHandling(*tool_use_event);

        if (std::holds_alternative<mojom::PermissionChallengePtr>(
                interaction_result)) {
          // Tool needs permission challenge acceptance before proceeding
          mojom::PermissionChallengePtr permission_challenge = std::move(
              std::get<mojom::PermissionChallengePtr>(interaction_result));
          CHECK(permission_challenge);
          DVLOG(1) << __func__ << " tool requires permission: "
                   << tool_use_event->tool_name;
          tool_use_event->permission_challenge =
              std::move(permission_challenge);
          OnToolUseEventOutput(last_entry.get(), tool_use_event.get());
          break;
        } else if (std::get<bool>(interaction_result)) {
          // Tool needs user to provide output via UI (e.g. UserChoiceTool)
          DVLOG(1) << __func__ << " tool requires user output: "
                   << tool_use_event->tool_name;
          break;
        }
        // interaction_result is false - user interaction can be automatic
      }

      // No user interaction needed - execute tool
      is_tool_use_in_progress_ = true;
      OnAPIRequestInProgressChanged();
      DVLOG(0) << __func__ << " calling UseTool for tool: " << tool_ptr->Name();

      tool_ptr->UseTool(
          tool_use_event->arguments_json,
          base::BindOnce(&ConversationHandler::RespondToToolUseRequest,
                         weak_ptr_factory_.GetWeakPtr(), tool_use_event->id));
      break;
    }
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
  return associated_content_manager_->HasAssociatedContent();
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

bool ConversationHandler::IsTemporaryChat() const {
  return metadata_->temporary;
}

void ConversationHandler::MaybeSwitchToVisionModel(
    const std::optional<std::vector<mojom::UploadedFilePtr>>& uploaded_files) {
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
}

void ConversationHandler::OnAutoScreenshotsTaken(
    base::OnceClosure callback,
    std::optional<std::vector<mojom::UploadedFilePtr>> screenshots) {
  // Apply MAX_IMAGES limit to automatic screenshots and calculate visual
  // percentage This function is only called for automatic screenshot capture,
  // never for user uploads
  std::optional<uint32_t> calculated_visual_percentage;

  if (screenshots && !screenshots->empty()) {
    size_t total_screenshots = screenshots->size();

    // Apply MAX_IMAGES limit for automatic screenshots
    if (total_screenshots > mojom::MAX_IMAGES) {
      size_t discarded_count = total_screenshots - mojom::MAX_IMAGES;

      // Calculate visual content percentage
      calculated_visual_percentage = base::checked_cast<uint32_t>(
          base::checked_cast<float>(mojom::MAX_IMAGES) * 100.0f /
          total_screenshots);

      // Truncate to MAX_IMAGES limit
      screenshots->resize(mojom::MAX_IMAGES);
      screenshots->shrink_to_fit();

      DVLOG(1) << "Image truncation details - Total captured: "
               << total_screenshots << ", Sent to model: " << mojom::MAX_IMAGES
               << ", Discarded: " << discarded_count;
    }

    // Auto-switch to vision model if needed
    MaybeSwitchToVisionModel(screenshots);

    // Update the last conversation turn with the uploaded files
    if (!chat_history_.empty() &&
        chat_history_.back()->character_type == mojom::CharacterType::HUMAN) {
      // Append screenshots to existing uploaded files instead of replacing them
      if (!chat_history_.back()->uploaded_files) {
        chat_history_.back()->uploaded_files.emplace();
      }
      chat_history_.back()->uploaded_files->insert(
          chat_history_.back()->uploaded_files->end(),
          std::make_move_iterator(screenshots->begin()),
          std::make_move_iterator(screenshots->end()));
      // Notify UI about the updated conversation turn
      OnHistoryUpdate(chat_history_.back().Clone());
    }
  }

  // Store the calculated visual content percentage for UI display
  const auto old = visual_content_used_percentage_;
  visual_content_used_percentage_ = calculated_visual_percentage;

  // Trigger UI state update only when visual content percentage has changed
  if (old != visual_content_used_percentage_) {
    OnStateForConversationEntriesChanged();
  }

  // Continue with the original callback
  std::move(callback).Run();
}

}  // namespace ai_chat

#undef STARTER_PROMPT
