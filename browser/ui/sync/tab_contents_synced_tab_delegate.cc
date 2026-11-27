// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>

#include "base/feature_list.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "content/public/browser/security_principal.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/browser/temporary_container.h"
#include "brave/components/containers/core/common/features.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace content {
class WebContents;
}  // namespace content

namespace {

#if BUILDFLAG(ENABLE_CONTAINERS)
bool IsInTemporaryContainer(content::WebContents* web_contents) {
  if (!base::FeatureList::IsEnabled(containers::features::kContainers)) {
    return false;
  }
  auto* site_instance = web_contents->GetSiteInstance();
  if (!site_instance) {
    return false;
  }
  const content::StoragePartitionConfig& storage_partition_config =
      site_instance->GetSecurityPrincipal().GetStoragePartitionConfig();
  return containers::IsContainersStoragePartition(storage_partition_config) &&
         containers::IsTemporaryContainerId(
             storage_partition_config.partition_name());
}
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

}  // namespace

// Forward-declared and called by the chromium_src
// tab_contents_synced_tab_delegate.cc override, at the start of
// TabContentsSyncedTabDelegate::ShouldSync(), when session sync decides
// whether to publish this tab's state. A returned value is used as the result
// without running upstream's checks; std::nullopt defers to upstream.
std::optional<bool> BraveShouldSyncTab(
    [[maybe_unused]] content::WebContents* web_contents) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  // Temporary containers are local-only and must not publish their tab state
  // to session sync.
  if (IsInTemporaryContainer(web_contents)) {
    return false;
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
  return std::nullopt;
}
