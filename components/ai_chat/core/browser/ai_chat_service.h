// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_SERVICE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_SERVICE_H_

#include <stddef.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/sequence_checker.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_database.h"
#include "brave/components/ai_chat/core/browser/ai_chat_feedback_api.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_bridge.h"
#include "brave/components/ai_chat/core/browser/tools/tool_provider_factory.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/untrusted_frame.mojom.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sync/model/data_type_controller_delegate.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace syncer {
class DataTypeControllerDelegate;
}  // namespace syncer

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
class TabTrackerService;
class AIChatMetrics;
class MemoryStorageTool;

bool CanAssociateContent(AssociatedContentDelegate* delegate);

// Main entry point for creating and consuming AI Chat conversations
class AIChatService : public KeyedService,
                      public mojom::Service,
                      public mojom::UntrustedService,
                      public ConversationHandler::Observer,
                      public mojom::TabDataObserver {
 public:
  using SkusServiceGetter =
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>;
  using GetSuggestedTopicsCallback = base::OnceCallback<void(
      base::expected<std::vector<std::string>, mojom::APIError>)>;
  using GetFocusTabsCallback = base::OnceCallback<void(
      base::expected<std::vector<std::string>, mojom::APIError>)>;

  AIChatService(
      ModelService* model_service,
      TabTrackerService* tab_tracker_service,
      std::unique_ptr<AIChatCredentialManager> ai_chat_credential_manager,
      PrefService* profile_prefs,
      AIChatMetrics* ai_chat_metrics,
      os_crypt_async::OSCryptAsync* os_crypt_async,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::string_view channel_string,
      base::FilePath profile_path,
      // Factories of ToolProviders from other layers
      std::vector<std::unique_ptr<ToolProviderFactory>>
          tool_provider_factories = {});

  ~AIChatService() override;
  AIChatService(const AIChatService&) = delete;
  AIChatService& operator=(const AIChatService&) = delete;

  mojo::PendingRemote<mojom::Service> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::Service> receiver);

  // KeyedService
  void Shutdown() override;

  // Returns true if a conversation has a live ConversationHandler.
  bool HasActiveConversationHandler(const std::string& uuid);

  // Creates a sync controller delegate for the AI Chat data type. Uses
  // ProxyDataTypeControllerDelegate to forward from UI thread to the
  // background sequence where the bridge lives.
  std::unique_ptr<syncer::DataTypeControllerDelegate>
  CreateSyncControllerDelegate();

  // ConversationHandler::Observer
  void OnRequestInProgressChanged(ConversationHandler* handler,
                                  bool in_progress) override;
  void OnConversationEntryAdded(
      ConversationHandler* handler,
      mojom::ConversationTurnPtr& entry,
      std::optional<PageContents> maybe_associated_content) override;
  void OnConversationEntryRemoved(ConversationHandler* handler,
                                  const std::string& entry_uuid) override;
  void OnToolUseEventOutput(ConversationHandler* handler,
                            const std::string& entry_uuid,
                            size_t event_order,
                            mojom::ToolUseEventPtr tool_use) override;
  void OnClientConnectionChanged(ConversationHandler* handler) override;
  void OnConversationTitleChanged(const std::string& conversation_uuid,
                                  const std::string& title) override;
  void OnConversationTokenInfoChanged(const std::string& conversation_uuid,
                                      uint64_t total_tokens,
                                      uint64_t trimmed_tokens) override;
  void OnAssociatedContentUpdated(ConversationHandler* handler) override;

  // mojom::TabDataObserver
  void TabDataChanged(std::vector<mojom::TabDataPtr> tab_data) override;

  // Adds new conversation and returns the handler
  ConversationHandler* CreateConversation();

  // Provides memory tool for testing
  MemoryStorageTool* GetMemoryToolForTesting();

  ConversationHandler* GetConversation(std::string_view uuid);
  void GetConversation(std::string_view conversation_uuid,
                       base::OnceCallback<void(ConversationHandler*)>);

  // Creates and owns a ConversationHandler if one hasn't been made for the
  // associated_content_id yet. |associated_content_id| should not be stored. It
  // is an ephemeral identifier for active browser content.
  ConversationHandler* GetOrCreateConversationHandlerForContent(
      int associated_content_id,
      base::WeakPtr<AssociatedContentDelegate> associated_content);

  // Creates and owns a new ConversationHandler and associated with the provided
  // content ID. |associated_content_id| should not be stored. It
  // is an ephemeral identifier for active browser content.
  ConversationHandler* CreateConversationHandlerForContent(
      int associated_content_id,
      base::WeakPtr<AssociatedContentDelegate> associated_content);

  // Removes all in-memory and persisted data for all conversations
  void DeleteConversations(std::optional<base::Time> begin_time = std::nullopt,
                           std::optional<base::Time> end_time = std::nullopt);

  // Remove only web-content data from conversations
  void DeleteAssociatedWebContent(
      std::optional<base::Time> begin_time = std::nullopt,
      std::optional<base::Time> end_time = std::nullopt,
      base::OnceCallback<void(bool)> callback = base::DoNothing());

  void OpenConversationWithStagedEntries(
      base::WeakPtr<AssociatedContentDelegate> associated_content,
      base::OnceClosure open_ai_chat);

  void MaybeAssociateContent(AssociatedContentDelegate* content,
                             const std::string& conversation_uuid);
  void AssociateOwnedContent(std::unique_ptr<AssociatedContentDelegate> content,
                             const std::string& conversation_uuid);
  void DisassociateContent(const mojom::AssociatedContentPtr& content,
                           const std::string& conversation_uuid);

  void GetFocusTabs(const std::vector<Tab>& tabs,
                    const std::string& topic,
                    GetFocusTabsCallback callback);
  void GetSuggestedTopics(const std::vector<Tab>& tabs,
                          GetSuggestedTopicsCallback callback);

  // mojom::Service
  void MarkAgreementAccepted() override;
  void EnableStoragePref() override;
  void DismissStorageNotice() override;
  void DismissPremiumPrompt() override;
  void GetSkills(GetSkillsCallback callback) override;
  void CreateSkill(const std::string& shortcut,
                   const std::string& prompt,
                   const std::optional<std::string>& model) override;
  void UpdateSkill(const std::string& id,
                   const std::string& shortcut,
                   const std::string& prompt,
                   const std::optional<std::string>& model) override;
  void DeleteSkill(const std::string& id) override;
  void GetConversations(GetConversationsCallback callback) override;
  void GetActionMenuList(GetActionMenuListCallback callback) override;
  void GetPremiumStatus(
      mojom::Service::GetPremiumStatusCallback callback) override;
  void DeleteConversation(const std::string& id) override;
  void RenameConversation(const std::string& id,
                          const std::string& new_name) override;
  void ConversationExists(const std::string& conversation_uuid,
                          ConversationExistsCallback callback) override;
  void BindConversation(
      const std::string& uuid,
      mojo::PendingReceiver<mojom::ConversationHandler> receiver,
      mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler)
      override;

  void BindMetrics(mojo::PendingReceiver<mojom::Metrics> metrics) override;
  void BindObserver(mojo::PendingRemote<mojom::ServiceObserver> ui,
                    mojom::Service::BindObserverCallback callback) override;
  void BindObserver(
      mojo::PendingRemote<mojom::UntrustedServiceObserver> observer,
      mojom::UntrustedService::BindObserverCallback callback) override;
  void BindUntrustedService(
      mojo::PendingReceiver<mojom::UntrustedService> receiver);

  bool GetIsContentAgentAllowed() const;
  void SetIsContentAgentAllowed(bool is_allowed);

  bool HasUserOptedIn();
  bool IsPremiumStatus();

  // Whether the feature and user preference for history storage is enabled
  bool IsAIChatHistoryEnabled();

  std::unique_ptr<EngineConsumer> GetDefaultAIEngine();
  std::unique_ptr<EngineConsumer> GetEngineForModel(
      const std::string& model_key);

  void SetCredentialManagerForTesting(
      std::unique_ptr<AIChatCredentialManager> credential_manager) {
    credential_manager_ = std::move(credential_manager);
  }
  AIChatCredentialManager* GetCredentialManagerForTesting() {
    return credential_manager_.get();
  }
  AIChatFeedbackAPI* GetFeedbackAPIForTesting() { return feedback_api_.get(); }

  size_t GetInMemoryConversationCountForTesting();

  EngineConsumer* GetTabOrganizationEngineForTesting() {
    return tab_organization_engine_.get();
  }

  void SetTabOrganizationEngineForTesting(
      std::unique_ptr<EngineConsumer> engine) {
    tab_organization_engine_ = std::move(engine);
  }

  void SetTabTrackerServiceForTesting(TabTrackerService* tab_tracker_service) {
    tab_tracker_service_ = tab_tracker_service;
  }

  void SetDatabaseForTesting(
      base::SequenceBound<std::unique_ptr<AIChatDatabase>> db) {
    ai_chat_db_ = std::move(db);
  }

 private:
  friend class AIChatServiceUnitTest;
  FRIEND_TEST_ALL_PREFIXES(AIChatServiceUnitTest,
                           CreateTabOrganizationEngineIfNeeded_InvalidModelKey);

  // Key is uuid
  using ConversationMap =
      std::map<std::string, mojom::ConversationPtr, std::less<>>;
  using ConversationMapCallback = base::OnceCallback<void(ConversationMap&)>;

  void MaybeInitStorage();
  // Called when the database encryptor is ready.
  void OnOsCryptAsyncReady(scoped_refptr<os_crypt_async::Encryptor> encryptor);
  void LoadConversationsLazy(ConversationMapCallback callback);
  void OnLoadConversationsLazyData(
      std::vector<mojom::ConversationPtr> conversations);
  void ReloadConversations(bool from_cancel = false);
  void OnConversationDataReceived(
      std::string conversation_uuid,
      base::OnceCallback<void(ConversationHandler*)> callback,
      mojom::ConversationArchivePtr data);

  void MaybeAssociateContent(
      ConversationHandler* conversation,
      int associated_content_id,
      base::WeakPtr<AssociatedContentDelegate> associated_content);

  // Determines whether a conversation could be unloaded.
  bool CanUnloadConversation(ConversationHandler* conversation);

  // If a conversation is unloadable, queues an event to unload it after a
  // delay. The delay is to allow for these situations:
  // - Primarily to guarantee that any references to the conversation during the
  // current stack frame will remain valid during the current stack frame.
  //   Solves this in a block:
  //       auto conversation = CreateConversation();
  //       conversation->SomeMethodThatTriggersMaybeUnload();
  //       /* conversation is unloaded */
  //       conversation->SomeOtherMethod(); // use after free!
  // - To give clients a chance to connect, which often happens in a separate
  // process, e.g. WebUI. This is not critical, but it avoids unloading and then
  // re-loading the conversation data whilst waiting for the UI to connect.
  void QueueMaybeUnloadConversation(ConversationHandler* conversation);

  // Unloads |conversation| if:
  // 1. It hasn't already been unloaded
  // 2. |CanUnloadConversation| is true
  void MaybeUnloadConversation(base::WeakPtr<ConversationHandler> conversation);
  void HandleFirstEntry(ConversationHandler* handler,
                        mojom::ConversationTurnPtr& entry,
                        std::optional<std::vector<std::string>> maybe_content,
                        mojom::ConversationPtr& conversation);
  void HandleNewEntry(
      ConversationHandler* handler,
      mojom::ConversationTurnPtr& entry,
      std::optional<std::vector<std::string>> maybe_associated_content,
      mojom::ConversationPtr& conversation);

  void OnUserOptedIn();
  void OnSkusServiceReceived(
      SkusServiceGetter getter,
      mojo::PendingRemote<skus::mojom::SkusService> service);
  void OnConversationListChanged();
  // Called on this sequence (UI thread) by AIChatSyncBridge after it
  // applies a batch of remote ADDs/UPDATEs/DELETEs. Refreshes the
  // in-memory conversation list and pushes new data into any active
  // ConversationHandler — stopping any in-flight LLM request or tool-use
  // loop on those handlers first, since their local state has been
  // superseded by the remote write.
  void OnRemoteSyncDataApplied();
  void OnConversationDataForRemoteSyncReload(
      const std::string& uuid,
      mojom::ConversationArchivePtr archive);
  void OnPremiumStatusReceived(GetPremiumStatusCallback callback,
                               mojom::PremiumStatus status,
                               mojom::PremiumInfoPtr info);
  void OnDataDeletedForDisabledStorage(bool success);
  mojom::ServiceStatePtr BuildState();
  void OnStateChanged();
  void OnSkillsChanged();
  void OnMemoryEnabledChanged();
  void InitializeTools();

  void CreateTabOrganizationEngineIfNeeded();
  void OnTabOrganizationModelPrefChanged();

  void OnSuggestedTopicsReceived(
      GetSuggestedTopicsCallback callback,
      base::expected<std::vector<std::string>, mojom::APIError> topics);
  void OnGetFocusTabs(
      GetFocusTabsCallback callback,
      base::expected<std::vector<std::string>, mojom::APIError> result);
  std::vector<std::unique_ptr<ToolProvider>>
  CreateToolProvidersForNewConversation();

  raw_ptr<ModelService> model_service_;
  raw_ptr<TabTrackerService> tab_tracker_service_;
  raw_ptr<PrefService> profile_prefs_;
  raw_ptr<AIChatMetrics> ai_chat_metrics_;
  raw_ptr<os_crypt_async::OSCryptAsync> os_crypt_async_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  PrefChangeRegistrar pref_change_registrar_;

  std::unique_ptr<AIChatFeedbackAPI> feedback_api_;
  std::unique_ptr<AIChatCredentialManager> credential_manager_;

  // Factories of ToolProviders from other layers
  std::vector<std::unique_ptr<ToolProviderFactory>> tool_provider_factories_;

  // Engine for tab organization, created on demand and owned by AIChatService.
  std::unique_ptr<ai_chat::EngineConsumer> tab_organization_engine_;

  // Memory tool that is available and shared across all conversations.
  std::unique_ptr<MemoryStorageTool> memory_tool_;

  base::FilePath profile_path_;

  // Storage for conversations
  base::SequenceBound<std::unique_ptr<AIChatDatabase>> ai_chat_db_;

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
  // TODO(djandries): If the above requirement for this map changes,
  // adjust the metrics that depend on loaded conversation state accordingly.
  std::map<std::string, std::unique_ptr<ConversationHandler>>
      conversation_handlers_;

  // Map associated content id (a.k.a navigation id) to conversation uuid. This
  // acts as a cache for back-navigation to find the most recent conversation
  // for that navigation. This should be periodically cleaned up by removing any
  // keys where the ConversationHandler has had a destroyed
  // associated_content_delegate_ for some time.
  std::map<int, std::string> content_conversations_;

  // Cached suggested topics for users to be focused on from the latest
  // GetSuggestedTopics call, would be cleared when there are tab data changes.
  std::vector<std::string> cached_focus_topics_;

  base::ScopedMultiSourceObservation<ConversationHandler,
                                     ConversationHandler::Observer>
      conversation_observations_{this};
  mojo::ReceiverSet<mojom::Service> receivers_;
  mojo::RemoteSet<mojom::ServiceObserver> observer_remotes_;

  mojo::ReceiverSet<mojom::UntrustedService> untrusted_service_receivers_;
  mojo::RemoteSet<mojom::UntrustedServiceObserver>
      untrusted_service_observer_remotes_;

  mojo::Receiver<mojom::TabDataObserver> tab_data_observer_receiver_{this};

  // AIChatCredentialManager / Skus does not provide an event when
  // subscription status changes. So we cache it and fetch latest fairly
  // often (whenever UI is focused).
  mojom::PremiumStatus last_premium_status_ = mojom::PremiumStatus::Unknown;

  // Whether conversations can utilize content agent capabilities. For now,
  // this is profile-specific.
  bool is_content_agent_allowed_ = false;

  // Background task runner for the database and sync bridge. Created
  // synchronously the first time MaybeInitStorage() succeeds so that a
  // ProxyDataTypeControllerDelegate can be handed out even before the bridge
  // has finished initializing on it.
  scoped_refptr<base::SequencedTaskRunner> db_task_runner_;

  // Thread-safe holder for the sync bridge so that
  // CreateSyncControllerDelegate() can hand out a working proxy delegate
  // before the bridge itself has been created on |db_task_runner_|. The
  // bridge is owned, set, accessed, and destroyed only on |db_task_runner_|.
  // The holder itself is shared between the UI thread (for binding into the
  // proxy delegate's callback) and |db_task_runner_| (for actual access).
  class SyncBackend : public base::RefCountedDeleteOnSequence<SyncBackend> {
   public:
    explicit SyncBackend(
        scoped_refptr<base::SequencedTaskRunner> owning_task_runner);

    // Called on the owning sequence to install the bridge once it has been
    // constructed. Must be called at most once.
    void SetBridge(std::unique_ptr<AIChatSyncBridge> bridge);

    // Returns the bridge's change-processor controller delegate, or a null
    // weak_ptr if the bridge has not been installed yet (or has been
    // released by Shutdown()). Called by ProxyDataTypeControllerDelegate
    // on the owning sequence.
    base::WeakPtr<syncer::DataTypeControllerDelegate> GetControllerDelegate();

    // Drops the bridge on the owning sequence so its raw_ptr to the
    // AIChatDatabase is released before AIChatService destroys the
    // database. SyncBackend itself may still be kept alive by proxy
    // delegates the sync engine holds; GetControllerDelegate() returns
    // null after this point.
    void Shutdown();

   private:
    friend class base::RefCountedDeleteOnSequence<SyncBackend>;
    friend class base::DeleteHelper<SyncBackend>;
    ~SyncBackend();

    std::unique_ptr<AIChatSyncBridge> bridge_;
    SEQUENCE_CHECKER(sequence_checker_);
  };
  // True while an os_crypt_async GetInstance() call is in flight, to
  // prevent MaybeInitStorage() from starting a second one (which would
  // race and cause a double bridge install when the second callback
  // fires).
  bool os_crypt_init_pending_ = false;

  scoped_refptr<SyncBackend> sync_backend_;

  // Weak ptr to the bridge owned by |sync_backend_|. Written on
  // |db_task_runner_| after the bridge has been constructed. Read on the
  // UI thread to fan outbound conversation notifications into bridge
  // method calls posted back to |db_task_runner_|; the weak_ptr check at
  // invocation time makes the race benign.
  base::WeakPtr<AIChatSyncBridge> sync_bridge_weak_;

  base::WeakPtrFactory<AIChatService> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_SERVICE_H_
