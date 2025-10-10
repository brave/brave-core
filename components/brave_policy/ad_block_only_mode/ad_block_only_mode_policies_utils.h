/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_POLICY_AD_BLOCK_ONLY_MODE_AD_BLOCK_ONLY_MODE_POLICIES_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_POLICY_AD_BLOCK_ONLY_MODE_AD_BLOCK_ONLY_MODE_POLICIES_UTILS_H_

namespace policy {
class PolicyBundle;
}  // namespace policy

void LoadAdBlockOnlyModePolicies(policy::PolicyBundle& bundle);

#endif  // BRAVE_COMPONENTS_BRAVE_POLICY_AD_BLOCK_ONLY_MODE_AD_BLOCK_ONLY_MODE_POLICIES_UTILS_H_
