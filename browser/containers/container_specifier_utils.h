// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_CONTAINERS_CONTAINER_SPECIFIER_UTILS_H_
#define BRAVE_BROWSER_CONTAINERS_CONTAINER_SPECIFIER_UTILS_H_

#include <optional>

#include "base/values.h"
#include "brave/components/containers/core/browser/container_specifier.h"

namespace content {
class BrowserContext;
class StoragePartitionConfig;
class WebContents;
}  // namespace content

namespace containers {

// Serializes the container specifier for the given web contents into the value.
void SerializeContainerSpecifier(content::WebContents* web_contents,
                                 base::Value& value);

// Deserializes the container specifier from the value into the container
// specifier.
void DeserializeContainerSpecifier(const base::Value& value,
                                   ContainerSpecifier& container_specifier);

// Returns the storage partition config for the given container specifier.
std::optional<content::StoragePartitionConfig>
GetStoragePartitionConfigForContainerSpecifier(
    content::BrowserContext* browser_context,
    const ContainerSpecifier& container_specifier);

}  // namespace containers

#endif  // BRAVE_BROWSER_CONTAINERS_CONTAINER_SPECIFIER_UTILS_H_
