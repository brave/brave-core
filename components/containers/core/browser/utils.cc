// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/utils.h"

#include "base/check.h"
#include "base/strings/strcat.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace containers {

namespace {

inline constexpr std::string_view kContainersStoragePartitionDomainPrefix =
    "containers-";

}  // namespace

bool IsContainersStoragePartitionDomain(std::string_view partition_domain) {
  return partition_domain.starts_with(kContainersStoragePartitionDomainPrefix);
}

std::string GetContainersStoragePartitionDomain(
    const mojom::ContainerPtr& container) {
  CHECK(container);
  return base::StrCat({kContainersStoragePartitionDomainPrefix, container->id});
}

std::string GetContainerIdFromContainersStoragePartitionDomain(
    std::string_view partition_domain) {
  CHECK(IsContainersStoragePartitionDomain(partition_domain));
  return std::string(
      partition_domain.substr(kContainersStoragePartitionDomainPrefix.size()));
}

}  // namespace containers
