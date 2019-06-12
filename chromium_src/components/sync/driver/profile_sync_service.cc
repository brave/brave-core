/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/components/sync/driver/profile_sync_service.h"

// For use_transport_only_mode
#define IsSyncFeatureEnabled IsBraveSyncEnabled
#include "../../../../components/sync/driver/profile_sync_service.cc"   // NOLINT

#include "base/bind.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "chrome/browser/sync/chrome_sync_client.h"
#include "content/public/browser/browser_thread.h"

namespace syncer {
const int64_t kBraveDefaultPollIntervalSeconds = 60;

void ProfileSyncService::BraveEngineParamsInit(
    syncer::SyncEngine::InitParams* params) {
  DCHECK(params);
  params->nudge_sync_cycle_delegate_function =
      base::BindRepeating(&ProfileSyncService::OnNudgeSyncCycle,
                          sync_enabled_weak_factory_.GetWeakPtr());
  params->poll_sync_cycle_delegate_function =
      base::BindRepeating(&ProfileSyncService::OnPollSyncCycle,
                          sync_enabled_weak_factory_.GetWeakPtr());

  sync_prefs_.SetPollInterval(
    base::TimeDelta::FromSeconds(
      syncer::kBraveDefaultPollIntervalSeconds));
}

void ProfileSyncService::OnNudgeSyncCycle(
    brave_sync::RecordsListPtr records_list) {}

void ProfileSyncService::OnPollSyncCycle(brave_sync::GetRecordsCallback cb,
                                         base::WaitableEvent* wevent) {}

bool ProfileSyncService::IsBraveSyncEnabled() const {
  return false;
}

}   // namespace syncer
