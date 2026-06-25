/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_data_type_controller.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/data_type.h"
#include "components/sync/model/data_type_controller_delegate.h"
#include "components/sync/model/proxy_data_type_controller_delegate.h"
#include "components/sync/service/sync_service.h"

namespace ai_chat {

AIChatDataTypeController::AIChatDataTypeController(
    syncer::SyncService* sync_service,
    PrefService* pref_service,
    AIChatServiceGetter ai_chat_service_getter)
    : DataTypeController(syncer::AI_CHAT_CONVERSATION),
      sync_service_(sync_service),
      pref_service_(pref_service),
      ai_chat_service_getter_(std::move(ai_chat_service_getter)) {
  auto create_delegate = [this] {
    return std::make_unique<syncer::ProxyDataTypeControllerDelegate>(
        base::SequencedTaskRunner::GetCurrentDefault(),
        base::BindRepeating(&AIChatDataTypeController::GetBridgeDelegate,
                            weak_ptr_factory_.GetWeakPtr()));
  };
  InitDataTypeController(/*delegate_for_full_sync_mode=*/create_delegate(),
                         /*delegate_for_transport_mode=*/create_delegate());

  pref_registrar_.Init(pref_service_);
  // base::Unretained() is safe because `pref_registrar_` is owned by `this`.
  pref_registrar_.Add(
      prefs::kBraveChatStorageEnabled,
      base::BindRepeating(&AIChatDataTypeController::OnStoragePrefChanged,
                          base::Unretained(this)));
  pref_registrar_.Add(
      prefs::kEnabledByPolicy,
      base::BindRepeating(&AIChatDataTypeController::OnPolicyPrefChanged,
                          base::Unretained(this)));

  // Connecting subscribes to the database-ready signal, without which a
  // database attached after sync first checks the precondition would never
  // have sync check again. The constructor can't connect while the SyncService
  // is still being built (see the class comment), so it posts the connect. That
  // task still runs before sync first checks the precondition, which waits for
  // the sync engine to start.
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&AIChatDataTypeController::ConnectToAIChatService,
                     weak_ptr_factory_.GetWeakPtr()));
}

AIChatDataTypeController::~AIChatDataTypeController() = default;

syncer::DataTypeController::PreconditionState
AIChatDataTypeController::GetPreconditionState(
    const PreconditionContext& context) const {
  // Repeats AIChatService::IsAIChatHistoryEnabled(), because there may be no
  // AIChatService to ask. Both parts matter: without the chat history feature,
  // AIChatService never opens a database, even though the storage pref
  // defaults to on, so the pref alone would leave the type waiting for one.
  if (!features::IsAIChatHistoryEnabled() ||
      !pref_service_->GetBoolean(prefs::kBraveChatStorageEnabled)) {
    return PreconditionState::kMustStopAndClearData;
  }
  // Checked here, because not every platform's getter returns null while a
  // policy disables AI Chat.
  if (!IsAIChatEnabled(pref_service_)) {
    return PreconditionState::kMustStopAndKeepData;
  }
  AIChatService* ai_chat_service = ai_chat_service_getter_.Run();
  if (!ai_chat_service || !ai_chat_service->IsSyncDatabaseReady()) {
    return PreconditionState::kMustStopAndKeepData;
  }
  return PreconditionState::kPreconditionsMet;
}

// static
base::WeakPtr<syncer::DataTypeControllerDelegate>
AIChatDataTypeController::GetBridgeDelegate(
    base::WeakPtr<AIChatDataTypeController> controller) {
  if (!controller) {
    return nullptr;
  }
  controller->ConnectToAIChatService();
  return controller->bridge_delegate_
             ? controller->bridge_delegate_->GetWeakPtr()
             : nullptr;
}

void AIChatDataTypeController::ConnectToAIChatService() {
  if (bridge_delegate_) {
    return;
  }
  AIChatService* ai_chat_service = ai_chat_service_getter_.Run();
  if (!ai_chat_service) {
    return;
  }
  bridge_delegate_ = ai_chat_service->CreateSyncControllerDelegate();
  // base::Unretained() is safe because `database_ready_subscription_` is owned
  // by `this` and unregisters the callback when destroyed.
  database_ready_subscription_ =
      ai_chat_service->RegisterSyncDatabaseReadyCallback(
          base::BindRepeating(&AIChatDataTypeController::OnSyncDatabaseReady,
                              base::Unretained(this)));
}

void AIChatDataTypeController::OnStoragePrefChanged() {
  // Turning storage on waits for OnSyncDatabaseReady() instead, as the
  // precondition isn't met until the database is attached.
  if (!pref_service_->GetBoolean(prefs::kBraveChatStorageEnabled)) {
    sync_service_->DataTypePreconditionChanged(type());
  }
}

void AIChatDataTypeController::OnPolicyPrefChanged() {
  // On desktop the getter returns null while a policy disables AI Chat, so a
  // policy enabling AI Chat after startup is when the controller can first
  // connect. Whichever way the policy changed, the precondition may have too.
  ConnectToAIChatService();
  sync_service_->DataTypePreconditionChanged(type());
}

void AIChatDataTypeController::OnSyncDatabaseReady() {
  sync_service_->DataTypePreconditionChanged(type());
}

}  // namespace ai_chat
