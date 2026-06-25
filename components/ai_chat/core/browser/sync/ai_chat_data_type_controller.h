/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_DATA_TYPE_CONTROLLER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_DATA_TYPE_CONTROLLER_H_

#include <memory>

#include "base/callback_list.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sync/service/data_type_controller.h"

class PrefService;

namespace syncer {
class DataTypeControllerDelegate;
class SyncService;
}  // namespace syncer

namespace ai_chat {

class AIChatService;

// Controls the AI_CHAT_CONVERSATION sync data type. Sync asks a type's
// controller whether the type can run (its precondition), and starts and stops
// the type through the controller's delegates, which here forward to the
// AIChatSyncBridge on the database sequence.
//
// SyncServiceFactory registers this controller whenever
// features::IsAIChatEnabled() and features::IsBraveSyncAIChatEnabled() are
// both on, whatever the chat history storage pref says, and even while a policy
// disables AI Chat. The set of registered types is fixed for the SyncService's
// lifetime, so a type left out at startup couldn't sync until the browser
// restarts. The precondition follows storage instead:
//  - Storage off: kMustStopAndClearData. Sync stops the type and clears its
//    sync metadata, so turning storage on again runs a first sync, which
//    downloads every conversation into the new, empty database.
//  - Storage on, but a policy disables AI Chat, the database isn't attached to
//    the bridge yet, or there's no AIChatService: kMustStopAndKeepData. A
//    first sync started before the attach would merge into no database. The
//    controller has sync check the precondition again when AIChatService
//    signals the attach, and when the policy changes.
//  - Otherwise kPreconditionsMet.
//
// SyncServiceFactory creates the controller while it builds the SyncService.
// On desktop, looking up AIChatService then would build it, and its
// dependencies (ProfileMiscMetricsService, then PersonalDataManager) would get
// a null SyncService from the factory still building it. So the controller is
// given a getter, which it runs only once the SyncService is built: in a task
// the constructor posts, in the precondition, and from its delegates.
// SyncServiceImpl::Initialize() calls the delegates before that build
// finishes, to clear the sync metadata of the types the user hasn't selected.
// So the delegates are ProxyDataTypeControllerDelegates, which post each call
// to this sequence and run the getter only in that task.
class AIChatDataTypeController : public syncer::DataTypeController {
 public:
  // Returns the profile's AIChatService, or null if AI Chat isn't available.
  using AIChatServiceGetter = base::RepeatingCallback<AIChatService*()>;

  AIChatDataTypeController(syncer::SyncService* sync_service,
                           PrefService* pref_service,
                           AIChatServiceGetter ai_chat_service_getter);

  AIChatDataTypeController(const AIChatDataTypeController&) = delete;
  AIChatDataTypeController& operator=(const AIChatDataTypeController&) = delete;

  ~AIChatDataTypeController() override;

  // syncer::DataTypeController:
  PreconditionState GetPreconditionState(
      const PreconditionContext& context) const override;

 private:
  // Provides the delegate that both proxy delegates forward to. Static, taking
  // the controller as an argument, because a callback bound to a WeakPtr
  // receiver must return void.
  static base::WeakPtr<syncer::DataTypeControllerDelegate> GetBridgeDelegate(
      base::WeakPtr<AIChatDataTypeController> controller);

  // Gets AIChatService's delegate for the sync bridge, and subscribes to the
  // database-ready signal. Does nothing if already done, or if there's no
  // AIChatService.
  void ConnectToAIChatService();

  void OnStoragePrefChanged();
  void OnPolicyPrefChanged();
  void OnSyncDatabaseReady();

  const raw_ptr<syncer::SyncService> sync_service_;
  const raw_ptr<PrefService> pref_service_;
  const AIChatServiceGetter ai_chat_service_getter_;

  PrefChangeRegistrar pref_registrar_;
  std::unique_ptr<syncer::DataTypeControllerDelegate> bridge_delegate_;
  base::CallbackListSubscription database_ready_subscription_;

  base::WeakPtrFactory<AIChatDataTypeController> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_DATA_TYPE_CONTROLLER_H_
