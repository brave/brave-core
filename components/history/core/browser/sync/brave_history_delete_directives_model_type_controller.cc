/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/history/core/browser/sync/brave_history_delete_directives_model_type_controller.h"

#include "components/history/core/browser/sync/history_delete_directives_model_type_controller.h"

namespace history {

BraveHistoryDeleteDirectivesModelTypeController::
    BraveHistoryDeleteDirectivesModelTypeController(
        const base::RepeatingClosure& dump_stack,
        syncer::SyncService* sync_service,
        syncer::ModelTypeStoreService* model_type_store_service,
        HistoryService* history_service,
        PrefService* pref_service)
    : HistoryDeleteDirectivesModelTypeController(dump_stack,
                                                 sync_service,
                                                 model_type_store_service,
                                                 history_service,
                                                 pref_service) {}

BraveHistoryDeleteDirectivesModelTypeController::
    ~BraveHistoryDeleteDirectivesModelTypeController() = default;

syncer::ModelTypeController::PreconditionState
BraveHistoryDeleteDirectivesModelTypeController::GetPreconditionState() const {
  DCHECK(CalledOnValidThread());
  return PreconditionState::kPreconditionsMet;
}

}  // namespace history
