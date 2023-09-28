/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/history/core/browser/sync/brave_history_model_type_controller.h"

#include "base/feature_list.h"
#include "components/sync/base/features.h"

namespace history {

BraveHistoryModelTypeController::BraveHistoryModelTypeController(
    syncer::ModelType model_type,
    syncer::SyncService* sync_service,
    signin::IdentityManager* identity_manager,
    HistoryService* history_service,
    PrefService* pref_service)
    : HistoryModelTypeController(model_type,
                                 sync_service,
                                 identity_manager,
                                 history_service,
                                 pref_service) {}

BraveHistoryModelTypeController::~BraveHistoryModelTypeController() = default;

syncer::DataTypeController::PreconditionState
BraveHistoryModelTypeController::GetPreconditionState() const {
  if (!base::FeatureList::IsEnabled(syncer::kSyncEnableHistoryDataType)) {
    DCHECK_EQ(type(), syncer::TYPED_URLS);
    return HistoryModelTypeController::GetPreconditionState();
  }

  // If the History feature flag is enabled, HISTORY replaces TYPED_URLS.
  if (type() == syncer::TYPED_URLS) {
    return PreconditionState::kMustStopAndClearData;
  }
  DCHECK_EQ(type(), syncer::HISTORY);
  return PreconditionState::kPreconditionsMet;
}

}  // namespace history
