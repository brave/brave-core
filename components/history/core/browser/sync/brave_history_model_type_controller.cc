/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/history/core/browser/sync/brave_history_model_type_controller.h"

namespace history {

BraveHistoryModelTypeController::BraveHistoryModelTypeController(
    syncer::SyncService* sync_service,
    signin::IdentityManager* identity_manager,
    HistoryService* history_service,
    PrefService* pref_service)
    : HistoryModelTypeController(sync_service,
                                 identity_manager,
                                 history_service,
                                 pref_service) {}

BraveHistoryModelTypeController::~BraveHistoryModelTypeController() = default;

syncer::ModelTypeController::PreconditionState
BraveHistoryModelTypeController::GetPreconditionState() const {
  return PreconditionState::kPreconditionsMet;
}

}  // namespace history
