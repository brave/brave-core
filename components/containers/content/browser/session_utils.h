// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_SESSION_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_SESSION_UTILS_H_

#include <optional>
#include <string>
#include <utility>

#include "base/component_export.h"
#include "url/gurl.h"

namespace containers {

COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::optional<std::string> GetVirtualUrlPrefix(
    const std::pair<std::string, std::string>& storage_partition_key);

COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::optional<GURL> RestoreStoragePartitionKeyFromUrl(
    const GURL& url,
    std::pair<std::string, std::string>& storage_partition_key,
    size_t& url_prefix_length);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_SESSION_UTILS_H_
