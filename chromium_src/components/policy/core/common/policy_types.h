// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_COMMON_POLICY_TYPES_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_COMMON_POLICY_TYPES_H_

// Inject a new POLICY_SOURCE_BRAVE at the end so it doesn't affect other
// integer values which could be stored in logs or sent via network
// serialization. This seems safer than using a -1 value which could be
// indexed, or using a larger value which could cause issues when
// indexing into arrays too.
#define POLICY_SOURCE_COUNT POLICY_SOURCE_BRAVE, POLICY_SOURCE_COUNT

// Define a new kBravePriority to be used as the new lowest priority
#define kEnterpriseDefault kBravePriority, kEnterpriseDefault

#include <components/policy/core/common/policy_types.h>  // IWYU pragma: export

// Static assert to ensure POLICY_SOURCE_BRAVE has the expected value of 10
// We're generally OK if this changes, but we want to be aware of when it does
// in case of any unexpected issues.  We believe that only log files would use
// a new integrer value if anything.
static_assert(policy::POLICY_SOURCE_BRAVE == 10,
              "POLICY_SOURCE_BRAVE must equal 10");

#undef kEnterpriseDefault
#undef POLICY_SOURCE_COUNT

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_COMMON_POLICY_TYPES_H_
