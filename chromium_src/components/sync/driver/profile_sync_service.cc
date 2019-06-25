/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/components/sync/driver/profile_sync_service.h"

#include <utility>

#include "brave/components/brave_sync/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "brave/components/brave_sync/public/brave_profile_sync_service.h"
#include "chrome/browser/sync/chrome_sync_client.h"
#include "content/public/browser/browser_thread.h"

using brave_sync::BraveProfileSyncService;
#endif

namespace syncer {

#if BUILDFLAG(ENABLE_BRAVE_SYNC)
const int64_t kBraveDefaultPollIntervalSeconds = 60;

bool IsBraveSyncEnabled(ProfileSyncService* profile_sync_service) {
  return static_cast<BraveProfileSyncService*>(
      profile_sync_service)->IsBraveSyncEnabled();
}

void OnNudgeSyncCycle(base::WeakPtr<ProfileSyncService> profile_sync_service,
                      brave_sync::RecordsListPtr records_list) {
  if (profile_sync_service.get()) {
    static_cast<BraveProfileSyncService*>(
        profile_sync_service.get())->OnNudgeSyncCycle(std::move(records_list));
  }
}

void OnPollSyncCycle(base::WeakPtr<ProfileSyncService> profile_sync_service,
                     brave_sync::GetRecordsCallback cb,
                     base::WaitableEvent* wevent) {
  if (profile_sync_service.get()) {
    static_cast<BraveProfileSyncService*>(
        profile_sync_service.get())->OnPollSyncCycle(cb, wevent);
  }
}
#endif

void BraveInit(
    base::WeakPtr<ProfileSyncService> profile_sync_service,
    SyncPrefs* sync_prefs,
    syncer::SyncEngine::InitParams* params) {
#if BUILDFLAG(ENABLE_BRAVE_SYNC)
  DCHECK(params);
  params->nudge_sync_cycle_delegate_function =
      base::BindRepeating(&OnNudgeSyncCycle,
                          profile_sync_service);
  params->poll_sync_cycle_delegate_function =
      base::BindRepeating(&OnPollSyncCycle,
                          profile_sync_service);

  sync_prefs->SetPollInterval(
      base::TimeDelta::FromSeconds(
          syncer::kBraveDefaultPollIntervalSeconds));
#endif
}

}   // namespace syncer

// avoid redefining IsSyncFeatureEnabled in header
#include "components/sync/driver/sync_service.h"
// For use_transport_only_mode
#define IsSyncFeatureEnabled() IsBraveSyncEnabled(this)
#define BRAVE_PROFILE_SYNC_SERVICE_START_UP_SLOW_ENGINE_COMPONENTS \
BraveInit(sync_enabled_weak_factory_.GetWeakPtr(), &sync_prefs_, &params);

#include "../../../../components/sync/driver/profile_sync_service.cc"   // NOLINT
