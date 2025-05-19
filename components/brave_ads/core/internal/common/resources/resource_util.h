/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_UTIL_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"

namespace brave_ads {

template <typename T>
using LoadAndParseResourceComponentCallback =
    base::OnceCallback<void(std::optional<T>)>;

template <typename T>
void LoadAndParseResourceComponent(
    const std::string& id,
    int version,
    LoadAndParseResourceComponentCallback<T> callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_UTIL_H_
