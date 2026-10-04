// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/sync/tab_contents_synced_tab_delegate.h"

#include "brave/components/containers/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/browser/temporary_container.h"
#include "brave/components/containers/core/common/features.h"
#include "content/public/browser/security_principal.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

#define ShouldSync ShouldSync_ChromiumImpl

#include <chrome/browser/ui/sync/tab_contents_synced_tab_delegate.cc>

#undef ShouldSync

bool TabContentsSyncedTabDelegate::ShouldSync(
    sync_sessions::SyncSessionsClient* sessions_client) {
  if (!ShouldSync_ChromiumImpl(sessions_client)) {
    return false;
  }

#if BUILDFLAG(ENABLE_CONTAINERS)
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    // Temporary containers are local-only and should never publish tab state to
    // session sync.
    if (auto* site_instance = web_contents()->GetSiteInstance()) {
      const content::StoragePartitionConfig& storage_partition_config =
          site_instance->GetSecurityPrincipal().GetStoragePartitionConfig();
      if (containers::IsContainersStoragePartition(storage_partition_config)) {
        return !containers::IsTemporaryContainerId(
            storage_partition_config.partition_name());
      }
    }
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
  return true;
}
