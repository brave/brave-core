/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_

#include "brave/browser/policy/brave_origin_policy_provider.h"

// Inject our BraveOrigin policy provider member alongside existing members
#define configuration_policy_provider_                     \
  configuration_policy_provider_;                          \
  std::unique_ptr<brave_origin::BraveOriginPolicyProvider> \
      brave_origin_policy_provider_

#include <chrome/browser/policy/profile_policy_connector.h>  // IWYU pragma: export
#undef configuration_policy_provider_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_POLICY_PROFILE_POLICY_CONNECTOR_H_
