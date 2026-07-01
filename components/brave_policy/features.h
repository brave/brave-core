/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_FEATURES_H_

#include "base/component_export.h"
#include "base/feature_list.h"

namespace brave_policy::features {

// Caches and restores pref values for policies that don't support dynamic
// refresh.
COMPONENT_EXPORT(POLICY_PREF_INTERCEPTOR)
BASE_DECLARE_FEATURE(kCacheNonDynamicPolicyPrefs);

}  // namespace brave_policy::features

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_FEATURES_H_
