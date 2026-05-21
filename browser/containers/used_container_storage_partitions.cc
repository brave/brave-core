// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/containers/used_container_storage_partitions.h"

#include "brave/browser/containers/containers_service_factory.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/browser/containers_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/storage_partition_config.h"

namespace containers {

std::vector<content::StoragePartitionConfig>
GetUsedContainerStoragePartitionConfigs(Profile* profile) {
  std::vector<content::StoragePartitionConfig> configs;
  if (!profile) {
    return configs;
  }

  ContainersService* service = ContainersServiceFactory::GetForProfile(profile);
  if (!service) {
    return configs;
  }

  for (const std::string& id : service->GetUsedContainerIds()) {
    configs.push_back(content::StoragePartitionConfig::Create(
        profile, kContainersStoragePartitionDomain, id,
        profile->IsOffTheRecord()));
  }

  return configs;
}

}  // namespace containers
