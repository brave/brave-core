// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_SERVICE_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "base/callback_list.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/ai_chat_feedback_api.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace os_crypt_async {
class Encryptor;
class OSCryptAsync;
}  // namespace os_crypt_async

class PrefService;
namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class ModelService;
class AIChatMetrics;

// Main entry point for creating and consuming AI Chat conversations
class AIChatService : public KeyedService,
                      public mojom::Service,
                      public ConversationHandler::Observer {
 public:
  using SkusServiceGetter =
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>;

  AIChatService(
      ModelService* model_service,
      std::unique_ptr<AIChatCredentialManager> ai_chat_credential_manager,
      PrefService* profile_prefs,
      AIChatMetrics* ai_chat_metrics,
      os_crypt_async::OSCryptAsync* os_crypt_async,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::string_view channel_string,
      base::FilePath profile_path);

  ~AIChatService() override;
  AIChatService(const AIChatService&) = delete;
  AIChatService& operator=(const AIChatService&) = delete;

  mojo::PendingRemote<mojom::Service> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::Service> receiver);

  // KeyedService
  void Shutdown() override;

  // ConversationHandler::Observer
  void OnRequestInProgressChanged(ConversationHandler* handler,
                                  bool in_progress) override;
  void OnConversationEntryAdded(
      ConversationHandler* handler,
      mojom::ConversationTurnPtr& entry,
      std::optional<std::string_view> associated_content_value) override;
  void OnConversationEntryRemoved(ConversationHandler* handler,
                                  std::string entry_uuid) override;
  void OnClientConnectionChanged(ConversationHandler* handler) override;
  void OnConversationTitleChanged(ConversationHandler* handler,
                                  std::string title) override;

  // Adds new conversation and returns the handler
  ConversationHandler* CreateConversation();

  ConversationHandler* GetConversation(std::string_view uuid);
  void GetConversation(std::string_view conversation_uuid,
                       base::OnceCallback<void(ConversationHandler*)>);

  // Creates and owns a ConversationHandler if one hasn't been made for the
  // associated_content_id yet. |associated_content_id| should not be stored. It
  // is an ephemeral identifier for active browser content.
  ConversationHandler* GetOrCreateConversationHandlerForContent(
      int associated_content_id,
      base::WeakPtr<ConversationHandler::AssociatedContentDelegate>
          associated_content);

  // Creates and owns a new ConversationHandler and associated with the provided
  // content ID. |associated_content_id| should not be stored. It
  // is an ephemeral identifier for active browser content.
  ConversationHandler* CreateConversationHandlerForContent(
      int associated_content_id,
      base::WeakPtr<ConversationHandler::AssociatedContentDelegate>
          associated_content);

  // Removes all in-memory and persisted data for all conversations
  void DeleteConversations(std::optional<base::Time> begin_time = std::nullopt,
                           std::optional<base::Time> end_time = std::nullopt);

  // Remove only web-content data from conversations
  void DeleteAssociatedWebContent(
      std::optional<base::Time> begin_time = std::nullopt,
      std::optional<base::Time> end_time = std::nullopt,
      base::OnceCallback<void(bool)> callback = base::DoNothing());

  void OpenConversationWithStagedEntries(
      base::WeakPtr<ConversationHandler::AssociatedContentDelegate>
          associated_content,
      base::OnceClosure open_ai_chat);

  // mojom::Service
  void MarkAgreementAccepted() override;
  void GetVisibleConversations(
      GetVisibleConversationsCallback callback) override;
  void GetActionMenuList(GetActionMenuListCallback callback) override;
  void GetPremiumStatus(GetPremiumStatusCallback callback) override;
  void GetCanShowPremiumPrompt(
      GetCanShowPremiumPromptCallback callback) override;
  void DismissPremiumPrompt() override;
  void DeleteConversation(const std::string& id) override;
  void RenameConversation(const std::string& id,
                          const std::string& new_name) override;

  void BindConversation(
      const std::string& uuid,
      mojo::PendingReceiver<mojom::ConversationHandler> receiver,
      mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler)
      override;
  void BindObserver(mojo::PendingRemote<mojom::ServiceObserver> ui) override;

  bool HasUserOptedIn();
  bool IsPremiumStatus();

  std::unique_ptr<EngineConsumer> GetDefaultAIEngine();

  AIChatCredentialManager* GetCredentialManagerForTesting() {
    return credential_manager_.get();
  }
  AIChatFeedbackAPI* GetFeedbackAPIForTesting() { return feedback_api_.get(); }

  size_t GetInMemoryConversationCountForTesting();

 private:
  // Key is uuid
  using ConversationMap = std::map<std::string, mojom::ConversationPtr>;
  using ConversationMapCallback = base::OnceCallback<void(ConversationMap&)>;

  void MaybeInitStorage();
  // Called when the database encryptor is ready.
  void OnOsCryptAsyncReady(os_crypt_async::Encryptor encryptor, bool success);
  void LoadConversationsLazy(ConversationMapCallback callback);
  void OnLoadConversationsLazyData(
      std::vector<mojom::ConversationPtr> conversations);
  void ReloadConversations(bool from_cancel = false);
  void OnConversationDataReceived(
      std::string conversation_uuid,
      base::OnceCallback<void(ConversationHandler*)> callback,
      mojom::ConversationArchivePtr data);

  void MaybeAssociateContentWithConversation(
      ConversationHandler* conversation,
      int associated_content_id,
      base::WeakPtr<ConversationHandler::AssociatedContentDelegate>
          associated_content);
  void MaybeUnloadConversation(ConversationHandler* conversation);
  void HandleFirstEntry(
      ConversationHandler* handler,
      mojom::ConversationTurnPtr& entry,
      std::optional<std::string_view> associated_content_value,
      mojom::ConversationPtr& conversation);
  void HandleNewEntry(ConversationHandler* handler,
                      mojom::ConversationTurnPtr& entry,
                      std::optional<std::string_view> associated_content_value,
                      mojom::ConversationPtr& conversation);

  void OnUserOptedIn();
  void OnSkusServiceReceived(
      SkusServiceGetter getter,
      mojo::PendingRemote<skus::mojom::SkusService> service);
  void OnConversationListChanged();
  void OnPremiumStatusReceived(GetPremiumStatusCallback callback,
                               mojom::PremiumStatus status,
                               mojom::PremiumInfoPtr info);
  void OnDataDeletedForDisabledStorage(bool success);

  bool IsAIChatHistoryEnabled();

  raw_ptr<ModelService> model_service_;
  raw_ptr<PrefService> profile_prefs_;
  raw_ptr<AIChatMetrics> ai_chat_metrics_;
  raw_ptr<os_crypt_async::OSCryptAsync> os_crypt_async_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  PrefChangeRegistrar pref_change_registrar_;

  std::unique_ptr<AIChatFeedbackAPI> feedback_api_;
  std::unique_ptr<AIChatCredentialManager> credential_manager_;

  base::FilePath profile_path_;

  // Storage for conversations
  base::SequenceBound<AIChatDatabase> ai_chat_db_;

  // nullopt if haven't started fetching, empty if done fetching
  std::optional<std::vector<ConversationMapCallback>>
      on_conversations_loaded_callbacks_;
  base::OnceClosure cancel_conversation_load_callback_ = base::NullCallback();

  // All conversation metadata. Mainly just titles and uuids.
  ConversationMap conversations_;

  // Only keep ConversationHandlers around that are being
  // actively used. Any metadata that needs to stay in-memory
  // should be kept in |conversations_|. Any other data only for viewing
  // conversation detail should be persisted to database.
  std::map<std::string, std::unique_ptr<ConversationHandler>>
      conversation_handlers_;

  // Map associated content id (a.k.a navigation id) to conversation uuid. This
  // acts as a cache for back-navigation to find the most recent conversation
  // for that navigation. This should be periodically cleaned up by removing any
  // keys where the ConversationHandler has had a destroyed
  // associated_content_delegate_ for some time.
  std::map<int, std::string> content_conversations_;

  base::ScopedMultiSourceObservation<ConversationHandler,
                                     ConversationHandler::Observer>
      conversation_observations_{this};
  mojo::ReceiverSet<mojom::Service> receivers_;
  mojo::RemoteSet<mojom::ServiceObserver> observer_remotes_;

  // AIChatCredentialManager / Skus does not provide an event when
  // subscription status changes. So we cache it and fetch latest fairly
  // often (whenever UI is focused).
  mojom::PremiumStatus last_premium_status_ = mojom::PremiumStatus::Unknown;
  // Maintains the subscription for `OSCryptAsync` and cancels upon destruction.
  base::CallbackListSubscription encryptor_ready_subscription_;

  base::WeakPtrFactory<AIChatService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_SERVICE_H_
