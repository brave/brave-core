/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_BRAVE_HISTORY_DATA_TYPE_CONTROLLER_H_
#define BRAVE_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_BRAVE_HISTORY_DATA_TYPE_CONTROLLER_H_

#include "components/history/core/browser/sync/history_data_type_controller.h"
#include "components/sync/base/model_type.h"

class PrefService;

namespace signin {
// class AccountManagedStatusFinder;
class IdentityManager;
}  // namespace signin

namespace syncer {
// class DataTypeStoreService;
class SyncService;
}  // namespace syncer

namespace history {

class HistoryService;

class BraveHistoryDataTypeController : public HistoryDataTypeController {
 public:
  BraveHistoryDataTypeController(syncer::SyncService* sync_service,
                                 signin::IdentityManager* identity_manager,
                                 HistoryService* history_service,
                                 PrefService* pref_service);

  BraveHistoryDataTypeController(const BraveHistoryDataTypeController&) =
      delete;
  BraveHistoryDataTypeController& operator=(
      const BraveHistoryDataTypeController&) = delete;

  ~BraveHistoryDataTypeController() override;

  // syncer::DataTypeController implementation.
  PreconditionState GetPreconditionState() const override;
};

}  // namespace history

#endif  // BRAVE_COMPONENTS_HISTORY_CORE_BROWSER_SYNC_BRAVE_HISTORY_DATA_TYPE_CONTROLLER_H_
