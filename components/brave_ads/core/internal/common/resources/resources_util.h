/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCES_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCES_UTIL_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_parsing_error_or.h"

namespace brave_ads {

template <typename T>
using LoadAndParseResourceCallback =
    base::OnceCallback<void(ResourceParsingErrorOr<T>)>;

template <typename T>
void LoadAndParseResource(const std::string& id,
                          int version,
                          LoadAndParseResourceCallback<T> callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCES_UTIL_H_
