/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_DATA_TYPE_CONTROLLER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_DATA_TYPE_CONTROLLER_H_

#include <memory>

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sync/service/data_type_controller.h"

class PrefService;

namespace syncer {
class DataTypeControllerDelegate;
class SyncService;
}  // namespace syncer

namespace ai_chat {

class AIChatService;

// DataTypeController for syncer::AI_CHAT_CONVERSATION.
//
// AI Chat conversations are only synced while on-disk conversation storage is
// enabled (prefs::kBraveChatStorageEnabled). This controller ties the data
// type's lifetime to that pref:
//   * storage off -> kMustStopAndClearData: the engine stops the type and
//     clears its sync metadata, so the wiped-and-recreated local database is
//     not left diverging from the engine's "everything is synced" view.
//   * storage on  -> kPreconditionsMet: the type runs normally.
// Toggling storage off then on therefore performs a fresh initial sync,
// re-downloading conversations from the server into the recreated database.
//
// The off->on transition is driven from AIChatService's "database ready"
// signal rather than directly from the pref change. On re-enable the database
// is re-attached to the sync bridge asynchronously (os_crypt -> SetDatabase),
// and restarting the type before that attach completes would let
// MergeFullSyncData run against a detached database and silently drop the
// merge. Waiting for RegisterSyncDatabaseReadyCallback() guarantees the
// database is attached before the engine re-evaluates the precondition. The
// off transition is applied immediately from the pref change, since there is
// nothing to wait for.
//
// See https://github.com/brave/brave-browser/issues/53978.
class AIChatDataTypeController : public syncer::DataTypeController {
 public:
  AIChatDataTypeController(syncer::SyncService* sync_service,
                           PrefService* pref_service,
                           AIChatService* ai_chat_service,
                           std::unique_ptr<syncer::DataTypeControllerDelegate>
                               delegate_for_full_sync_mode,
                           std::unique_ptr<syncer::DataTypeControllerDelegate>
                               delegate_for_transport_mode);

  AIChatDataTypeController(const AIChatDataTypeController&) = delete;
  AIChatDataTypeController& operator=(const AIChatDataTypeController&) = delete;

  ~AIChatDataTypeController() override;

  // syncer::DataTypeController:
  PreconditionState GetPreconditionState(
      const PreconditionContext& context) const override;

 private:
  // Invoked when kBraveChatStorageEnabled changes. Only the off transition is
  // acted on here; the on transition waits for OnSyncDatabaseReady() (see the
  // class comment).
  void OnStoragePrefChanged();
  // Invoked when AIChatService (re)attaches the database to the sync bridge.
  void OnSyncDatabaseReady();

  const raw_ptr<syncer::SyncService> sync_service_;
  const raw_ptr<PrefService> pref_service_;

  PrefChangeRegistrar pref_registrar_;
  base::CallbackListSubscription database_ready_subscription_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_SYNC_AI_CHAT_DATA_TYPE_CONTROLLER_H_
