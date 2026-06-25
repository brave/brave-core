/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_data_type_controller.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/data_type.h"
#include "components/sync/service/sync_service.h"

namespace ai_chat {

AIChatDataTypeController::AIChatDataTypeController(
    syncer::SyncService* sync_service,
    PrefService* pref_service,
    AIChatService* ai_chat_service,
    std::unique_ptr<syncer::DataTypeControllerDelegate>
        delegate_for_full_sync_mode,
    std::unique_ptr<syncer::DataTypeControllerDelegate>
        delegate_for_transport_mode)
    : DataTypeController(syncer::AI_CHAT_CONVERSATION,
                         std::move(delegate_for_full_sync_mode),
                         std::move(delegate_for_transport_mode)),
      sync_service_(sync_service),
      pref_service_(pref_service) {
  pref_registrar_.Init(pref_service_);
  // base::Unretained() is safe because `pref_registrar_` is owned by `this`.
  pref_registrar_.Add(
      prefs::kBraveChatStorageEnabled,
      base::BindRepeating(&AIChatDataTypeController::OnStoragePrefChanged,
                          base::Unretained(this)));
  // base::Unretained() is safe because `database_ready_subscription_` is owned
  // by `this` and unregisters the callback when destroyed.
  database_ready_subscription_ =
      ai_chat_service->RegisterSyncDatabaseReadyCallback(
          base::BindRepeating(&AIChatDataTypeController::OnSyncDatabaseReady,
                              base::Unretained(this)));
}

AIChatDataTypeController::~AIChatDataTypeController() = default;

syncer::DataTypeController::PreconditionState
AIChatDataTypeController::GetPreconditionState(
    const PreconditionContext& context) const {
  return pref_service_->GetBoolean(prefs::kBraveChatStorageEnabled)
             ? PreconditionState::kPreconditionsMet
             : PreconditionState::kMustStopAndClearData;
}

void AIChatDataTypeController::OnStoragePrefChanged() {
  // Only act on the off transition here. On re-enable the database is
  // re-attached asynchronously; OnSyncDatabaseReady() re-evaluates the
  // precondition once the attach has happened, so the restarted type's initial
  // sync sees an attached database.
  if (!pref_service_->GetBoolean(prefs::kBraveChatStorageEnabled)) {
    sync_service_->DataTypePreconditionChanged(type());
  }
}

void AIChatDataTypeController::OnSyncDatabaseReady() {
  sync_service_->DataTypePreconditionChanged(type());
}

}  // namespace ai_chat
