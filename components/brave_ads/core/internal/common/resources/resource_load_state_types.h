/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_LOAD_STATE_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_LOAD_STATE_TYPES_H_

namespace brave_ads {

enum class ResourceLoadStateType {
  // Not yet loaded.
  kNotLoaded,

  // Exists, but failed to open, read, or parse.
  kFailedToLoad,

  // Exists and loaded successfully.
  kLoaded,
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_RESOURCES_RESOURCE_LOAD_STATE_TYPES_H_
