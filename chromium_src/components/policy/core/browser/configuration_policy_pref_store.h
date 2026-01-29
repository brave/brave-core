/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_BROWSER_CONFIGURATION_POLICY_PREF_STORE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_BROWSER_CONFIGURATION_POLICY_PREF_STORE_H_

#include "brave/components/brave_policy/policy_pref_interceptor.h"

#define handler_list_ \
  handler_list_;      \
  brave_policy::PolicyPrefInterceptor pref_interceptor_

#include <components/policy/core/browser/configuration_policy_pref_store.h>  // IWYU pragma: export

#undef handler_list_

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_POLICY_CORE_BROWSER_CONFIGURATION_POLICY_PREF_STORE_H_
