/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_COMMON_FEATURES_H_

#include "base/feature_list.h"

namespace web_discovery::features {

// Enables the native re-implementation of the Web Discovery Project.
// If enabled, the Web Discovery component of the extension should be disabled.
BASE_DECLARE_FEATURE(kWebDiscoveryNative);

}  // namespace web_discovery::features

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_COMMON_FEATURES_H_
