// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/storage_partition_utils.h"

#include "base/check.h"
#include "base/strings/strcat.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace containers {

namespace {

inline constexpr std::string_view kContainerStoragePartitionDomainPrefix =
    "container-";

}  // namespace

bool IsContainerStoragePartitionDomain(std::string_view partition_domain) {
  return partition_domain.starts_with(kContainerStoragePartitionDomainPrefix);
}

std::optional<std::string_view> GetContainerIdFromStoragePartitionDomain(
    std::string_view partition_domain) {
  if (!IsContainerStoragePartitionDomain(partition_domain)) {
    return std::nullopt;
  }

  return partition_domain.substr(kContainerStoragePartitionDomainPrefix.size());
}

std::string GetContainerStoragePartitionDomain(
    const mojom::ContainerPtr& container) {
  CHECK(container);
  return base::StrCat({kContainerStoragePartitionDomainPrefix, container->id});
}

}  // namespace containers
