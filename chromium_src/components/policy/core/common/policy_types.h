// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_COMMON_POLICY_TYPES_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_COMMON_POLICY_TYPES_H_

// Inject a new POLICY_SOURCE_BRAVE_ORIGIN at the end so it doesn't affect other
// integer values which could be stored in logs or sent via network
// serialization
#define POLICY_SOURCE_COUNT POLICY_SOURCE_BRAVE_ORIGIN, POLICY_SOURCE_COUNT

// Define a new kBraveOriginPriority to be used as the new lowest priority
#define kEnterpriseDefault kBraveOriginPriority, kEnterpriseDefault

#include <components/policy/core/common/policy_types.h>  // IWYU pragma: export

#undef kEnterpriseDefault
#undef POLICY_SOURCE_COUNT

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_COMMON_POLICY_TYPES_H_
