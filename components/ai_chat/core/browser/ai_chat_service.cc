// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/ai_chat_service.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace ai_chat {
namespace {

constexpr auto kAllowedSchemes = base::MakeFixedFlatSet<std::string_view>(
    {url::kHttpsScheme, url::kHttpScheme, url::kFileScheme, url::kDataScheme});

}  // namespace

AIChatService::AIChatService(
    ModelService* model_service,
    std::unique_ptr<AIChatCredentialManager> ai_chat_credential_manager,
    PrefService* profile_prefs,
    AIChatMetrics* ai_chat_metrics,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::string_view channel_string)
    : model_service_(model_service),
      profile_prefs_(profile_prefs),
      ai_chat_metrics_(ai_chat_metrics),
      url_loader_factory_(url_loader_factory),
      feedback_api_(
          std::make_unique<AIChatFeedbackAPI>(url_loader_factory_,
                                              std::string(channel_string))),
      credential_manager_(std::move(ai_chat_credential_manager)) {
  DCHECK(profile_prefs_);
  pref_change_registrar_.Init(profile_prefs_);
  pref_change_registrar_.Add(
      prefs::kLastAcceptedDisclaimer,
      base::BindRepeating(&AIChatService::OnUserOptedIn,
                          weak_ptr_factory_.GetWeakPtr()));
}

AIChatService::~AIChatService() = default;

mojo::PendingRemote<mojom::Service> AIChatService::MakeRemote() {
  mojo::PendingRemote<mojom::Service> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void AIChatService::Bind(mojo::PendingReceiver<mojom::Service> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void AIChatService::Shutdown() {
  // Disconnect remotes
  receivers_.ClearWithReason(0, "Shutting down");
}

ConversationHandler* AIChatService::CreateConversation() {
  base::Uuid uuid = base::Uuid::GenerateRandomV4();
  std::string conversation_uuid = uuid.AsLowercaseString();
  // Create the conversation metadata
  {
    mojom::ConversationPtr conversation = mojom::Conversation::New(
        conversation_uuid, "", base::Time::Now(), false);
    conversations_.insert_or_assign(conversation_uuid, std::move(conversation));
  }
  mojom::Conversation* conversation =
      conversations_.at(conversation_uuid).get();
  // Create the ConversationHandler. We don't persist it until it has data.
  std::unique_ptr<ConversationHandler> conversation_handler =
      std::make_unique<ConversationHandler>(
          conversation, this, model_service_, credential_manager_.get(),
          feedback_api_.get(), url_loader_factory_);
  conversation_observations_.AddObservation(conversation_handler.get());

  // Own it
  conversation_handlers_.insert_or_assign(conversation_uuid,
                                          std::move(conversation_handler));

  DVLOG(1) << "Created conversation " << conversation_uuid << "\nNow have "
           << conversations_.size() << " conversations and "
           << conversation_handlers_.size() << " loaded in memory.";

  // TODO(petemill): Is this neccessary? This conversation won't be
  // considered visible until it has entries.
  OnConversationListChanged();

  // Return the raw pointer
  return GetConversation(conversation_uuid);
}

ConversationHandler* AIChatService::GetConversation(
    const std::string& conversation_uuid) {
  auto conversation_handler_it = conversation_handlers_.find(conversation_uuid);
  if (conversation_handler_it == conversation_handlers_.end()) {
    return nullptr;
  }
  return conversation_handler_it->second.get();
}

ConversationHandler* AIChatService::GetOrCreateConversationHandlerForContent(
    int associated_content_id,
    base::WeakPtr<ConversationHandler::AssociatedContentDelegate>
        associated_content) {
  ConversationHandler* conversation = nullptr;
  auto conversation_uuid_it =
      content_conversations_.find(associated_content_id);
  if (conversation_uuid_it != content_conversations_.end()) {
    auto conversation_uuid = conversation_uuid_it->second;
    // Load from memory or database, but probably not database as if the
    // conversation is in the associated content map then it's probably recent
    // and still in memory.
    conversation = GetConversation(conversation_uuid);
  }
  if (!conversation) {
    // New conversation needed
    conversation = CreateConversation();
  }
  // Provide the content delegate, if allowed
  MaybeAssociateContentWithConversation(conversation, associated_content_id,
                                        associated_content);

  return conversation;
}

ConversationHandler* AIChatService::CreateConversationHandlerForContent(
    int associated_content_id,
    base::WeakPtr<ConversationHandler::AssociatedContentDelegate>
        associated_content) {
  ConversationHandler* conversation = CreateConversation();
  // Provide the content delegate, if allowed
  MaybeAssociateContentWithConversation(conversation, associated_content_id,
                                        associated_content);

  return conversation;
}

void AIChatService::MaybeAssociateContentWithConversation(
    ConversationHandler* conversation,
    int associated_content_id,
    base::WeakPtr<ConversationHandler::AssociatedContentDelegate>
        associated_content) {
  if (associated_content &&
      kAllowedSchemes.contains(associated_content->GetURL().scheme())) {
    conversation->SetAssociatedContentDelegate(associated_content);
  }
  // Record that this is the latest conversation for this content. Even
  // if we don't call SetAssociatedContentDelegate, the conversation still
  // has a default Tab's navigation for which is is associated. The Conversation
  // won't use that Tab's Page for context.
  content_conversations_.insert_or_assign(
      associated_content_id, conversation->get_conversation_uuid());
}

void AIChatService::MarkAgreementAccepted() {
  SetUserOptedIn(profile_prefs_, true);
}

void AIChatService::GetActionMenuList(GetActionMenuListCallback callback) {
  std::move(callback).Run(ai_chat::GetActionMenuList());
}

void AIChatService::GetPremiumStatus(GetPremiumStatusCallback callback) {
  credential_manager_->GetPremiumStatus(
      base::BindOnce(&AIChatService::OnPremiumStatusReceived,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatService::GetCanShowPremiumPrompt(
    GetCanShowPremiumPromptCallback callback) {
  bool has_user_dismissed_prompt =
      profile_prefs_->GetBoolean(prefs::kUserDismissedPremiumPrompt);

  if (has_user_dismissed_prompt) {
    std::move(callback).Run(false);
    return;
  }

  base::Time last_accepted_disclaimer =
      profile_prefs_->GetTime(prefs::kLastAcceptedDisclaimer);

  // Can't show if we haven't accepted disclaimer yet
  if (last_accepted_disclaimer.is_null()) {
    std::move(callback).Run(false);
    return;
  }

  base::Time time_1_day_ago = base::Time::Now() - base::Days(1);
  bool is_more_than_24h_since_last_seen =
      last_accepted_disclaimer < time_1_day_ago;

  if (is_more_than_24h_since_last_seen) {
    std::move(callback).Run(true);
    return;
  }

  std::move(callback).Run(false);
}

void AIChatService::DismissPremiumPrompt() {
  profile_prefs_->SetBoolean(prefs::kUserDismissedPremiumPrompt, true);
}

void AIChatService::DeleteConversation(const std::string& id) {
  ConversationHandler* conversation_handler =
      conversation_handlers_.at(id).get();
  if (!conversation_handler) {
    return;
  }
  conversation_observations_.RemoveObservation(conversation_handler);
  conversation_handlers_.erase(id);
  conversations_.erase(id);
  DVLOG(1) << "Erased conversation due to deletion request (" << id
           << "). Now have " << conversations_.size()
           << " Conversation metadata items and "
           << conversation_handlers_.size()
           << " ConversationHandler instances.";
  OnConversationListChanged();
}

void AIChatService::RenameConversation(const std::string& id,
                                       const std::string& new_name) {
  ConversationHandler* conversation_handler =
      conversation_handlers_.at(id).get();
  if (!conversation_handler) {
    return;
  }

  DVLOG(1) << "Renamed conversation " << id << " to '" << new_name << "'";
  OnConversationTitleChanged(conversation_handler, new_name);
}

void AIChatService::OnPremiumStatusReceived(GetPremiumStatusCallback callback,
                                            mojom::PremiumStatus status,
                                            mojom::PremiumInfoPtr info) {
#if BUILDFLAG(IS_ANDROID)
  // There is no UI for android to "refresh" with an iAP - we are likely still
  // authenticating after first iAP, so we should show as active.
  if (status == mojom::PremiumStatus::ActiveDisconnected &&
      profile_prefs_->GetBoolean(prefs::kBraveChatSubscriptionActiveAndroid)) {
    status = mojom::PremiumStatus::Active;
  }
#endif

  last_premium_status_ = status;
  if (ai_chat::HasUserOptedIn(profile_prefs_) && ai_chat_metrics_ != nullptr) {
    ai_chat_metrics_->OnPremiumStatusUpdated(false, status, std::move(info));
  }
  model_service_->OnPremiumStatus(status);
  std::move(callback).Run(status, std::move(info));
}

void AIChatService::MaybeEraseConversation(
    ConversationHandler* conversation_handler) {
  // Don't unload if there is active UI for the conversation
  if (conversation_handler->IsAnyClientConnected()) {
    return;
  }

  bool has_history = conversation_handler->HasAnyHistory();

  // We can keep a conversation with history in memory until there is no active
  // content.
  // TODO(petemill): With the history feature enabled, we should unload (if
  // there is no request in progress). However, we can only do this when
  // GetOrCreateConversationHandlerForContent allows a callback so that it
  // can provide an answer after loading the conversation content from storage.
  if (conversation_handler->IsAssociatedContentAlive() && has_history) {
    return;
  }

  // AIChatHistory feature doesn't yet have persistant storage, so keep
  // handlers and data around if it's enabled.
  if (!features::IsAIChatHistoryEnabled() || !has_history) {
    // Can erase because no active UI and no history, so it's
    // not a real / persistable conversation
    auto uuid = conversation_handler->get_conversation_uuid();
    conversation_observations_.RemoveObservation(conversation_handler);
    conversation_handlers_.erase(uuid);
    conversations_.erase(uuid);
    std::erase_if(content_conversations_,
                  [&uuid](const auto& kv) { return kv.second == uuid; });
    DVLOG(1) << "Erased conversation (" << uuid << "). Now have "
             << conversations_.size() << " Conversation metadata items and "
             << conversation_handlers_.size()
             << " ConversationHandler instances.";
    OnConversationListChanged();
  }
}

void AIChatService::OnConversationEntriesChanged(
    ConversationHandler* handler,
    std::vector<mojom::ConversationTurnPtr> entries) {
  auto conversation_it = conversations_.find(handler->get_conversation_uuid());
  CHECK(conversation_it != conversations_.end());
  auto& conversation = conversation_it->second;
  if (!entries.empty()) {
    // This conversation is visible once the first response begins
    conversation->has_content = true;
    if (ai_chat_metrics_ != nullptr) {
      // Each time the user starts a conversation
      if (entries.size() == 1) {
        ai_chat_metrics_->RecordNewChat();
      }
      // Each time the user submits an entry
      if (entries.back()->character_type == mojom::CharacterType::HUMAN) {
        ai_chat_metrics_->RecordNewPrompt();
      }
    }
    OnConversationListChanged();
    // TODO(petemill): Persist the entries, but consider receiving finer grained
    // entry update events.
  }
}

void AIChatService::OnClientConnectionChanged(ConversationHandler* handler) {
  DVLOG(4) << "Client connection changed for conversation "
           << handler->get_conversation_uuid();
  MaybeEraseConversation(handler);
}

void AIChatService::OnConversationTitleChanged(ConversationHandler* handler,
                                               std::string title) {
  auto conversation_it = conversations_.find(handler->get_conversation_uuid());
  CHECK(conversation_it != conversations_.end());
  auto& conversation = conversation_it->second;
  conversation->title = title;
  OnConversationListChanged();
}

void AIChatService::OnAssociatedContentDestroyed(ConversationHandler* handler,
                                                 int content_id) {
  content_conversations_.erase(content_id);
  MaybeEraseConversation(handler);
}

void AIChatService::GetVisibleConversations(
    GetVisibleConversationsCallback callback) {
  std::vector<mojom::ConversationPtr> conversations;
  for (const auto& conversation : FilterVisibleConversations()) {
    conversations.push_back(conversation->Clone());
  }
  std::move(callback).Run(std::move(conversations));
}

void AIChatService::BindConversation(
    const std::string& uuid,
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  ConversationHandler* conversation = GetConversation(uuid);
  if (!conversation) {
    return;
  }
  CHECK(conversation) << "Asked to bind a conversation which doesn't exist";
  conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
}

void AIChatService::BindObserver(
    mojo::PendingRemote<mojom::ServiceObserver> observer) {
  observer_remotes_.Add(std::move(observer));
}

bool AIChatService::HasUserOptedIn() {
  return ai_chat::HasUserOptedIn(profile_prefs_);
}

bool AIChatService::IsPremiumStatus() {
  return ai_chat::IsPremiumStatus(last_premium_status_);
}

std::unique_ptr<EngineConsumer> AIChatService::GetDefaultAIEngine() {
  return model_service_->GetEngineForModel(model_service_->GetDefaultModelKey(),
                                           url_loader_factory_,
                                           credential_manager_.get());
}

size_t AIChatService::GetInMemoryConversationCountForTesting() {
  return conversation_handlers_.size();
}

void AIChatService::OnUserOptedIn() {
  bool is_opted_in = HasUserOptedIn();
  if (!is_opted_in) {
    return;
  }
  for (auto& kv : conversation_handlers_) {
    kv.second->OnUserOptedIn();
  }
  for (auto& remote : observer_remotes_) {
    remote->OnAgreementAccepted();
  }
  if (ai_chat_metrics_ != nullptr) {
    ai_chat_metrics_->RecordEnabled(true, true, {});
  }
}

std::vector<mojom::Conversation*> AIChatService::FilterVisibleConversations() {
  std::vector<mojom::Conversation*> conversations;
  for (const auto& kv : conversations_) {
    auto& conversation = kv.second;
    // Conversations are only visible if they have content
    if (!conversation->has_content) {
      continue;
    }
    conversations.push_back(conversation.get());
  }
  base::ranges::sort(conversations, std::greater<>(),
                     &mojom::Conversation::created_time);
  return conversations;
}

void AIChatService::OnConversationListChanged() {
  auto conversations = FilterVisibleConversations();
  for (auto& remote : observer_remotes_) {
    std::vector<mojom::ConversationPtr> client_conversations;
    for (const auto& conversation : conversations) {
      client_conversations.push_back(conversation->Clone());
    }
    remote->OnConversationListChanged(std::move(client_conversations));
  }
}

void AIChatService::OpenConversationWithStagedEntries(
    base::WeakPtr<ConversationHandler::AssociatedContentDelegate>
        associated_content,
    base::OnceClosure open_ai_chat) {
  if (!associated_content || !associated_content->HasOpenAIChatPermission()) {
    return;
  }

  ConversationHandler* conversation = GetOrCreateConversationHandlerForContent(
      associated_content->GetContentId(), associated_content);
  CHECK(conversation);

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  if (ai_chat_metrics_ != nullptr) {
    ai_chat_metrics_->HandleOpenViaEntryPoint(EntryPoint::kBraveSearch);
  }
#endif
  // Open AI Chat and trigger a fetch of staged conversations from Brave Search.
  std::move(open_ai_chat).Run();
  conversation->MaybeFetchOrClearContentStagedConversation();
}

}  // namespace ai_chat
