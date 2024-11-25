// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/ai_chat_service.h"

#include <array>
#include <compare>
#include <functional>
#include <ios>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/adapters.h"
#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/numerics/clamped_math.h"
#include "base/ranges/algorithm.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/types/strong_alias.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "build/build_config.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {
namespace {

constexpr base::FilePath::StringPieceType kDBFileName =
    FILE_PATH_LITERAL("AIChat");

constexpr auto kAllowedSchemes = base::MakeFixedFlatSet<std::string_view>(
    {url::kHttpsScheme, url::kHttpScheme, url::kFileScheme, url::kDataScheme});

std::vector<mojom::Conversation*> FilterVisibleConversations(
    std::map<std::string, mojom::ConversationPtr>& conversations_map) {
  std::vector<mojom::Conversation*> conversations;
  for (const auto& kv : conversations_map) {
    auto& conversation = kv.second;
    // Conversations are only visible if they have content
    if (!conversation->has_content) {
      continue;
    }
    conversations.push_back(conversation.get());
  }
  base::ranges::sort(conversations, std::greater<>(),
                     &mojom::Conversation::updated_time);
  return conversations;
}

bool IsConversationUpdatedTimeWithinRange(
    std::optional<base::Time> begin_time,
    std::optional<base::Time> end_time,
    mojom::ConversationPtr& conversation) {
  return ((!begin_time.has_value() || begin_time->is_null() ||
           conversation->updated_time >= begin_time) &&
          (!end_time.has_value() || end_time->is_null() || end_time->is_max() ||
           conversation->updated_time <= end_time));
}

}  // namespace

AIChatService::AIChatService(
    ModelService* model_service,
    std::unique_ptr<AIChatCredentialManager> ai_chat_credential_manager,
    PrefService* profile_prefs,
    AIChatMetrics* ai_chat_metrics,
    os_crypt_async::OSCryptAsync* os_crypt_async,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::string_view channel_string,
    base::FilePath profile_path)
    : model_service_(model_service),
      profile_prefs_(profile_prefs),
      ai_chat_metrics_(ai_chat_metrics),
      os_crypt_async_(os_crypt_async),
      url_loader_factory_(url_loader_factory),
      feedback_api_(
          std::make_unique<AIChatFeedbackAPI>(url_loader_factory_,
                                              std::string(channel_string))),
      credential_manager_(std::move(ai_chat_credential_manager)),
      profile_path_(profile_path) {
  DCHECK(profile_prefs_);
  pref_change_registrar_.Init(profile_prefs_);
  pref_change_registrar_.Add(
      prefs::kLastAcceptedDisclaimer,
      base::BindRepeating(&AIChatService::OnUserOptedIn,
                          weak_ptr_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(
      prefs::kStorageEnabled,
      base::BindRepeating(&AIChatService::MaybeInitStorage,
                          weak_ptr_factory_.GetWeakPtr()));

  MaybeInitStorage();
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
  weak_ptr_factory_.InvalidateWeakPtrs();
  if (ai_chat_db_) {
    ai_chat_db_.Reset();
  }
}

ConversationHandler* AIChatService::CreateConversation() {
  base::Uuid uuid = base::Uuid::GenerateRandomV4();
  std::string conversation_uuid = uuid.AsLowercaseString();
  // Create the conversation metadata
  {
    mojom::ConversationPtr conversation = mojom::Conversation::New(
        conversation_uuid, "", base::Time::Now(), false, std::nullopt,
        mojom::SiteInfo::New(base::Uuid::GenerateRandomV4().AsLowercaseString(),
                             mojom::ContentType::PageContent, std::nullopt,
                             std::nullopt, std::nullopt, 0, false, false));
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
    std::string_view conversation_uuid) {
  auto conversation_handler_it =
      conversation_handlers_.find(conversation_uuid.data());
  if (conversation_handler_it == conversation_handlers_.end()) {
    return nullptr;
  }
  return conversation_handler_it->second.get();
}

void AIChatService::GetConversation(
    std::string_view conversation_uuid,
    base::OnceCallback<void(ConversationHandler*)> callback) {
  if (ConversationHandler* cached_conversation =
          GetConversation(conversation_uuid)) {
    DVLOG(4) << __func__ << " found cached conversation for "
             << conversation_uuid;
    std::move(callback).Run(cached_conversation);
    return;
  }
  // Load from database
  if (!ai_chat_db_) {
    std::move(callback).Run(nullptr);
    return;
  }
  LoadConversationsLazy(base::BindOnce(
      [](base::WeakPtr<AIChatService> instance, std::string conversation_uuid,
         base::OnceCallback<void(ConversationHandler*)> callback,
         ConversationMap& conversations) {
        auto conversation_it = conversations.find(conversation_uuid);
        if (conversation_it == conversations.end()) {
          std::move(callback).Run(nullptr);
          return;
        }
        mojom::ConversationPtr& metadata = conversation_it->second;
        // Get archive content and conversation entries
        instance->ai_chat_db_.AsyncCall(&AIChatDatabase::GetConversationData)
            .WithArgs(metadata->uuid)
            .Then(base::BindOnce(&AIChatService::OnConversationDataReceived,
                                 std::move(instance), metadata->uuid,
                                 std::move(callback)));
      },
      weak_ptr_factory_.GetWeakPtr(), std::string(conversation_uuid),
      std::move(callback)));
}

void AIChatService::OnConversationDataReceived(
    std::string conversation_uuid,
    base::OnceCallback<void(ConversationHandler*)> callback,
    mojom::ConversationArchivePtr data) {
  DVLOG(4) << __func__ << " for " << conversation_uuid
           << " with data: " << data->entries.size() << " entries and "
           << data->associated_content.size() << " contents";
  auto conversation_it = conversations_.find(conversation_uuid);
  if (conversation_it == conversations_.end()) {
    std::move(callback).Run(nullptr);
    return;
  }
  mojom::Conversation* conversation = conversation_it->second.get();
  std::unique_ptr<ConversationHandler> conversation_handler =
      std::make_unique<ConversationHandler>(
          conversation, this, model_service_, credential_manager_.get(),
          feedback_api_.get(), url_loader_factory_, std::move(data));
  conversation_observations_.AddObservation(conversation_handler.get());
  conversation_handlers_.insert_or_assign(conversation_uuid,
                                          std::move(conversation_handler));
  std::move(callback).Run(GetConversation(conversation_uuid));
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

void AIChatService::DeleteConversations(std::optional<base::Time> begin_time,
                                        std::optional<base::Time> end_time) {
  if (!begin_time.has_value() && !end_time.has_value()) {
    // Delete all conversations
    // Delete in-memory data
    conversation_observations_.RemoveAllObservations();
    conversation_handlers_.clear();
    conversations_.clear();
    content_conversations_.clear();

    // Delete database data
    if (ai_chat_db_) {
      ai_chat_db_.AsyncCall(base::IgnoreResult(&AIChatDatabase::DeleteAllData));
      ReloadConversations();
    }
    if (ai_chat_metrics_ != nullptr) {
      ai_chat_metrics_->RecordReset();
    }
    OnConversationListChanged();
    return;
  }

  // Get all keys from conversations_
  std::vector<std::string> conversation_keys;

  for (auto& [uuid, conversation] : conversations_) {
    if (IsConversationUpdatedTimeWithinRange(begin_time, end_time,
                                             conversation)) {
      conversation_keys.push_back(uuid);
    }
  }

  for (const auto& uuid : conversation_keys) {
    DeleteConversation(uuid);
  }
  if (!conversation_keys.empty()) {
    OnConversationListChanged();
  }
}

void AIChatService::DeleteAssociatedWebContent(
    std::optional<base::Time> begin_time,
    std::optional<base::Time> end_time,
    base::OnceCallback<void(bool)> callback) {
  if (!ai_chat_db_) {
    std::move(callback).Run(true);
    return;
  }

  ai_chat_db_.AsyncCall(&AIChatDatabase::DeleteAssociatedWebContent)
      .WithArgs(begin_time, end_time)
      .Then(std::move(callback));

  // Update local data
  ReloadConversations();
}

void AIChatService::MaybeInitStorage() {
  if (IsAIChatHistoryEnabled()) {
    if (!ai_chat_db_) {
      DVLOG(0) << "Initializing OS Crypt Async";
      encryptor_ready_subscription_ = os_crypt_async_->GetInstance(
          base::BindOnce(&AIChatService::OnOsCryptAsyncReady,
                         weak_ptr_factory_.GetWeakPtr()));
      // Don't init DB until oscrypt is ready - we don't want to use the DB
      // if we can't use encryption.
    }
  } else {
    // Delete all stored data from database
    if (ai_chat_db_) {
      DVLOG(0) << "Unloading AI Chat database due to pref change";
      base::SequenceBound<AIChatDatabase> ai_chat_db = std::move(ai_chat_db_);
      ai_chat_db.AsyncCall(&AIChatDatabase::DeleteAllData)
          .Then(base::BindOnce(&AIChatService::OnDataDeletedForDisabledStorage,
                               weak_ptr_factory_.GetWeakPtr()));
    }
  }
}

void AIChatService::OnOsCryptAsyncReady(os_crypt_async::Encryptor encryptor,
                                        bool success) {
  CHECK(features::IsAIChatHistoryEnabled());
  if (!success) {
    LOG(ERROR) << "Failed to initialize AIChat DB due to OSCrypt failure";
    return;
  }
  // Pref might have changed since we started this process
  if (!profile_prefs_->GetBoolean(prefs::kStorageEnabled)) {
    return;
  }
  ai_chat_db_ = base::SequenceBound<AIChatDatabase>(
      base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::WithBaseSyncPrimitives(),
           base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN}),
      profile_path_.Append(kDBFileName), std::move(encryptor));
}

void AIChatService::OnDataDeletedForDisabledStorage(bool success) {
  // Remove any conversations from in-memory that aren't connected to UI.
  // This is done now, in the callback from DeleteAllData, in case there
  // was any in-progress operations that would have resulted in adding data
  // back to conversations_ whilst waiting for DeleteAllData to complete.
  std::vector<ConversationHandler*> all_conversation_handlers;
  for (auto& [_, conversation_handler] : conversation_handlers_) {
    all_conversation_handlers.push_back(conversation_handler.get());
  }
  for (auto* conversation_handler : all_conversation_handlers) {
    MaybeUnloadConversation(conversation_handler);
  }
  // Remove any conversation metadata that isn't connected to a still-alive
  // handler.
  for (auto it = conversations_.begin(); it != conversations_.end();) {
    if (!conversation_handlers_.contains(it->first)) {
      it = conversations_.erase(it);
    } else {
      ++it;
    }
  }
  OnConversationListChanged();
  // Re-check the preference since it could have been re-enabled
  // whilst the database operation was in progress. If so, we can re-use
  // the same database instance (post data deletion).
  if (!IsAIChatHistoryEnabled()) {
    // If there is a LoadConversationsLazy in progress, it will get cancelled
    // on destruction of ai_chat_db_ so call the callbacks.
    if (on_conversations_loaded_callbacks_.has_value() &&
        !on_conversations_loaded_callbacks_->empty()) {
      for (auto& callback : on_conversations_loaded_callbacks_.value()) {
        std::move(callback).Run(conversations_);
      }
    }

    ai_chat_db_.Reset();
    cancel_conversation_load_callback_ = base::NullCallback();
    on_conversations_loaded_callbacks_ = std::nullopt;
  }
}

void AIChatService::LoadConversationsLazy(ConversationMapCallback callback) {
  // Send immediately if we have finished loading from storage
  if (!ai_chat_db_ || (on_conversations_loaded_callbacks_.has_value() &&
                       on_conversations_loaded_callbacks_->empty())) {
    std::move(callback).Run(conversations_);
    return;
  }
  if (on_conversations_loaded_callbacks_.has_value()) {
    on_conversations_loaded_callbacks_->push_back(std::move(callback));
    return;
  }

  on_conversations_loaded_callbacks_ = std::vector<ConversationMapCallback>();
  on_conversations_loaded_callbacks_->push_back(std::move(callback));
  ai_chat_db_.AsyncCall(&AIChatDatabase::GetAllConversations)
      .Then(base::BindOnce(&AIChatService::OnLoadConversationsLazyData,
                           weak_ptr_factory_.GetWeakPtr()));
}

void AIChatService::OnLoadConversationsLazyData(
    std::vector<mojom::ConversationPtr> conversations) {
  if (!cancel_conversation_load_callback_.is_null()) {
    std::move(cancel_conversation_load_callback_).Run();
    cancel_conversation_load_callback_ = base::NullCallback();
    return;
  }
  DVLOG(1) << "Loaded " << conversations.size() << " conversations.";
  for (auto& conversation : conversations) {
    std::string uuid = conversation->uuid;
    DVLOG(2) << "Loaded conversation " << conversation->uuid
             << " with details: " << "\n has content: "
             << conversation->has_content
             << "\n last updated: " << conversation->updated_time
             << "\n title: " << conversation->title;
    // It's ok to overwrite existing metadata - some operations may modify
    // the database data and we want to keep the in-memory data synchronised.
    auto existing_conversation_it = conversations_.find(uuid);
    if (existing_conversation_it != conversations_.end()) {
      auto& existing_conversation = existing_conversation_it->second;
      existing_conversation->title = conversation->title;
      existing_conversation->updated_time = conversation->updated_time;
      existing_conversation->has_content = conversation->has_content;
      existing_conversation->model_key = conversation->model_key;
      existing_conversation->associated_content =
          std::move(conversation->associated_content);
    } else {
      conversations_.emplace(uuid, std::move(conversation));
    }
    auto handler_it = conversation_handlers_.find(uuid);
    if (handler_it != conversation_handlers_.end()) {
      // Notify the handler that metadata is possibly changed
      ConversationHandler* handler = handler_it->second.get();
      handler->OnConversationMetadataUpdated();
      // If a reload was asked for, then we should also update the deeper
      // conversation data from the database, since the reload was likely due
      // to underlying data changing.
      ai_chat_db_.AsyncCall(&AIChatDatabase::GetConversationData)
          .WithArgs(uuid)
          .Then(base::BindOnce(
              [](base::WeakPtr<ConversationHandler> handler,
                 mojom::ConversationArchivePtr updated_data) {
                if (!handler) {
                  return;
                }
                handler->OnArchiveContentUpdated(std::move(updated_data));
              },
              handler->GetWeakPtr()));
    }
  }
  if (on_conversations_loaded_callbacks_.has_value()) {
    for (auto& callback : on_conversations_loaded_callbacks_.value()) {
      std::move(callback).Run(conversations_);
    }
    on_conversations_loaded_callbacks_->clear();
  }
  OnConversationListChanged();
}

void AIChatService::ReloadConversations(bool from_cancel) {
  // If in the middle of a conversation load, then make sure data is ignored,
  // and ask again when current load is complete.
  if (!from_cancel && on_conversations_loaded_callbacks_.has_value() &&
      !on_conversations_loaded_callbacks_->empty()) {
    cancel_conversation_load_callback_ =
        base::BindOnce(&AIChatService::ReloadConversations,
                       weak_ptr_factory_.GetWeakPtr(), true);
    return;
  }

  // Collect any previous callbacks and force conversations to load again
  std::vector<ConversationMapCallback> previous_callbacks;
  if (on_conversations_loaded_callbacks_.has_value()) {
    on_conversations_loaded_callbacks_->swap(previous_callbacks);
  }
  on_conversations_loaded_callbacks_ = std::nullopt;
  LoadConversationsLazy(base::DoNothing());

  // Re-queue any previous callbacks
  for (auto& callback : previous_callbacks) {
    LoadConversationsLazy(std::move(callback));
  }
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
  auto handler_it = conversation_handlers_.find(id);
  if (handler_it != conversation_handlers_.end()) {
    conversation_observations_.RemoveObservation(handler_it->second.get());
    conversation_handlers_.erase(id);
  }
  conversations_.erase(id);
  DVLOG(1) << "Erased conversation due to deletion request (" << id
           << "). Now have " << conversations_.size()
           << " Conversation metadata items and "
           << conversation_handlers_.size()
           << " ConversationHandler instances.";
  OnConversationListChanged();
  // Update database
  if (ai_chat_db_) {
    ai_chat_db_
        .AsyncCall(base::IgnoreResult(&AIChatDatabase::DeleteConversation))
        .WithArgs(id);
  }
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
    ai_chat_metrics_->OnPremiumStatusUpdated(false, status, info.Clone());
  }
  model_service_->OnPremiumStatus(status);
  std::move(callback).Run(status, std::move(info));
}

void AIChatService::MaybeUnloadConversation(
    ConversationHandler* conversation_handler) {
  if (!conversation_handler->IsAnyClientConnected() &&
      !conversation_handler->IsRequestInProgress()) {
    // Can erase handler because no active UI
    bool has_history = conversation_handler->HasAnyHistory();
    auto uuid = conversation_handler->get_conversation_uuid();
    conversation_observations_.RemoveObservation(conversation_handler);
    conversation_handlers_.erase(uuid);
    DVLOG(1) << "Unloaded conversation (" << uuid << ") from memory. Now have "
             << conversations_.size() << " Conversation metadata items and "
             << conversation_handlers_.size()
             << " ConversationHandler instances.";
    if (!IsAIChatHistoryEnabled() || !has_history) {
      // Can erase because no active UI and no history, so it's
      // not a real / persistable conversation
      conversations_.erase(uuid);
      std::erase_if(content_conversations_,
                    [&uuid](const auto& kv) { return kv.second == uuid; });
      DVLOG(1) << "Erased conversation (" << uuid << "). Now have "
               << conversations_.size() << " Conversation metadata items and "
               << conversation_handlers_.size()
               << " ConversationHandler instances.";
      OnConversationListChanged();
    }
  } else {
    DVLOG(4) << "Not unloading conversation ("
             << conversation_handler->get_conversation_uuid()
             << ") from memory. Has active clients: "
             << (conversation_handler->IsAnyClientConnected() ? "yes" : "no")
             << " Request is in progress: "
             << (conversation_handler->IsRequestInProgress() ? "yes" : "no");
  }
}

bool AIChatService::IsAIChatHistoryEnabled() {
  return (features::IsAIChatHistoryEnabled() &&
          profile_prefs_->GetBoolean(prefs::kStorageEnabled));
}

void AIChatService::OnRequestInProgressChanged(ConversationHandler* handler,
                                               bool in_progress) {
  // We don't unload a conversation if it has a request in progress, so check
  // again when that changes.
  if (!in_progress) {
    MaybeUnloadConversation(handler);
  }
}

void AIChatService::OnConversationEntryAdded(
    ConversationHandler* handler,
    mojom::ConversationTurnPtr& entry,
    std::optional<std::string_view> associated_content_value) {
  auto conversation_it = conversations_.find(handler->get_conversation_uuid());
  CHECK(conversation_it != conversations_.end());
  mojom::ConversationPtr& conversation = conversation_it->second;

  if (!conversation->has_content) {
    HandleFirstEntry(handler, entry, associated_content_value, conversation);
  } else {
    HandleNewEntry(handler, entry, associated_content_value, conversation);
  }

  conversation->has_content = true;
  conversation->updated_time = entry->created_time;
  OnConversationListChanged();
}

void AIChatService::HandleFirstEntry(
    ConversationHandler* handler,
    mojom::ConversationTurnPtr& entry,
    std::optional<std::string_view> associated_content_value,
    mojom::ConversationPtr& conversation) {
  DVLOG(1) << __func__ << " Conversation " << conversation->uuid
           << " being persisted for first time.";
  CHECK(entry->uuid.has_value());
  CHECK(conversation->associated_content->uuid.has_value());
  // We can persist the conversation metadata for the first time as well as the
  // entry.
  if (ai_chat_db_) {
    ai_chat_db_.AsyncCall(base::IgnoreResult(&AIChatDatabase::AddConversation))
        .WithArgs(conversation->Clone(),
                  std::optional<std::string>(associated_content_value),
                  entry->Clone());
  }
  // Record metrics
  if (ai_chat_metrics_ != nullptr) {
    if (handler->GetConversationHistory().size() == 1) {
      ai_chat_metrics_->RecordNewChat();
    }
  }
}

void AIChatService::HandleNewEntry(
    ConversationHandler* handler,
    mojom::ConversationTurnPtr& entry,
    std::optional<std::string_view> associated_content_value,
    mojom::ConversationPtr& conversation) {
  CHECK(entry->uuid.has_value());
  DVLOG(1) << __func__ << " Conversation " << conversation->uuid
           << " persisting new entry. Count of entries: "
           << handler->GetConversationHistory().size();

  // Persist the new entry and update the associated content data, if present
  if (ai_chat_db_) {
    ai_chat_db_
        .AsyncCall(base::IgnoreResult(&AIChatDatabase::AddConversationEntry))
        .WithArgs(handler->get_conversation_uuid(), entry.Clone(),
                  conversation->model_key, std::nullopt);

    if (associated_content_value.has_value() &&
        conversation->associated_content->is_content_association_possible) {
      ai_chat_db_
          .AsyncCall(
              base::IgnoreResult(&AIChatDatabase::AddOrUpdateAssociatedContent))
          .WithArgs(conversation->uuid,
                    conversation->associated_content->Clone(),
                    std::optional<std::string>(associated_content_value));
    }
  }

  // Record metrics
  if (ai_chat_metrics_ != nullptr &&
      entry->character_type == mojom::CharacterType::HUMAN) {
    ai_chat_metrics_->RecordNewPrompt();
  }
}

void AIChatService::OnConversationEntryRemoved(ConversationHandler* handler,
                                               std::string entry_uuid) {
  // Persist the removal
  if (ai_chat_db_) {
    ai_chat_db_
        .AsyncCall(base::IgnoreResult(&AIChatDatabase::DeleteConversationEntry))
        .WithArgs(entry_uuid);
  }
}

void AIChatService::OnClientConnectionChanged(ConversationHandler* handler) {
  DVLOG(4) << "Client connection changed for conversation "
           << handler->get_conversation_uuid();
  MaybeUnloadConversation(handler);
}

void AIChatService::OnConversationTitleChanged(ConversationHandler* handler,
                                               std::string title) {
  auto conversation_it = conversations_.find(handler->get_conversation_uuid());
  CHECK(conversation_it != conversations_.end());
  auto& conversation = conversation_it->second;
  conversation->title = title;

  OnConversationListChanged();

  // Persist the change
  if (ai_chat_db_) {
    ai_chat_db_
        .AsyncCall(base::IgnoreResult(&AIChatDatabase::UpdateConversationTitle))
        .WithArgs(handler->get_conversation_uuid(), std::move(title));
  }
}

void AIChatService::GetVisibleConversations(
    GetVisibleConversationsCallback callback) {
  LoadConversationsLazy(base::BindOnce(
      [](GetVisibleConversationsCallback callback,
         ConversationMap& conversations_map) {
        std::vector<mojom::ConversationPtr> conversations;
        for (const auto& conversation :
             FilterVisibleConversations(conversations_map)) {
          conversations.push_back(conversation->Clone());
        }
        std::move(callback).Run(std::move(conversations));
      },
      std::move(callback)));
}

void AIChatService::BindConversation(
    const std::string& uuid,
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  GetConversation(
      std::move(uuid),
      base::BindOnce(
          [](mojo::PendingReceiver<mojom::ConversationHandler> receiver,
             mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler,
             ConversationHandler* handler) {
            if (!handler) {
              DVLOG(0) << "Failed to get conversation for binding";
              return;
            }
            handler->Bind(std::move(receiver),
                          std::move(conversation_ui_handler));
          },
          std::move(receiver), std::move(conversation_ui_handler)));
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

void AIChatService::OnConversationListChanged() {
  auto conversations = FilterVisibleConversations(conversations_);
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
