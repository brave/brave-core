/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_BRAVE_HISTORY_DELETE_DIRECTIVES_MODEL_TYPE_CONTROLLER_H_
#define BRAVE_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_BRAVE_HISTORY_DELETE_DIRECTIVES_MODEL_TYPE_CONTROLLER_H_

#include "base/functional/callback_forward.h"
#include "components/history/core/browser/sync/history_delete_directives_model_type_controller.h"
#include "components/sync/service/data_type_controller.h"

class PrefService;

namespace syncer {
class ModelTypeStoreService;
class SyncService;
}  // namespace syncer

namespace history {

class BraveHistoryDeleteDirectivesModelTypeController
    : public HistoryDeleteDirectivesModelTypeController {
 public:
  BraveHistoryDeleteDirectivesModelTypeController(
      const base::RepeatingClosure& dump_stack,
      syncer::SyncService* sync_service,
      syncer::ModelTypeStoreService* model_type_store_service,
      HistoryService* history_service,
      PrefService* pref_service);

  BraveHistoryDeleteDirectivesModelTypeController(
      const BraveHistoryDeleteDirectivesModelTypeController&) = delete;
  BraveHistoryDeleteDirectivesModelTypeController& operator=(
      const BraveHistoryDeleteDirectivesModelTypeController&) = delete;

  ~BraveHistoryDeleteDirectivesModelTypeController() override;

  // DataTypeController overrides.
  PreconditionState GetPreconditionState() const override;
};

}  // namespace history

#endif  // BRAVE_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_BRAVE_HISTORY_DELETE_DIRECTIVES_MODEL_TYPE_CONTROLLER_H_
