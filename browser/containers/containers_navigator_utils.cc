// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/containers/containers_navigator_utils.h"

#include <optional>

#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "content/public/browser/security_principal.h"

struct NavigateParams;

namespace containers {

std::optional<content::StoragePartitionConfig>
GetStoragePartitionConfigToInherit(const NavigateParams& params) {
  if (!base::FeatureList::IsEnabled(features::kContainers)) {
    return std::nullopt;
  }

  if (params.storage_partition_config) {
    return containers::MaybeInheritStoragePartition(
        *params.storage_partition_config);
  }

  if (params.source_site_instance) {
    return containers::MaybeInheritStoragePartition(
        params.source_site_instance->GetSecurityPrincipal()
            .GetStoragePartitionConfig());
  }

  return std::nullopt;
}

}  // namespace containers
