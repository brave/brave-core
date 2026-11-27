/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/values.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_user_settings.h"

namespace {

// IsSetupInProgress isn't accurate in brave sync flow especially for the first
// time setup, we rely on it to display setup dialog
void AddBraveSyncStatus(base::DictValue& sync_status,
                        syncer::SyncService* service,
                        bool disallowed_by_policy) {
  sync_status.Set(
      "firstSetupInProgress",
      service && !disallowed_by_policy &&
          !service->GetUserSettings()->IsInitialSyncFeatureSetupComplete());
  syncer::BraveSyncServiceImpl* brave_sync_service =
      static_cast<syncer::BraveSyncServiceImpl*>(service);
  using GetSeedStatusEnum = syncer::BraveSyncServiceImpl::GetSeedStatusEnum;
  if (brave_sync_service) {
    auto seed = brave_sync_service->GetSeed();
    sync_status.Set(
        "hasSyncWordsDecryptionError",
        !seed.has_value() && seed.error() == GetSeedStatusEnum::kDecryptFailed);
    sync_status.Set("isOsEncryptionAvailable",
                    brave_sync_service->IsEncryptionAvailable());
  }
}

}  // namespace

#include <chrome/browser/ui/webui/settings/people_handler.cc>
