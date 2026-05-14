/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_POLICY_PREF_INTERCEPTOR_UTILS_H_
#define BRAVE_BROWSER_POLICY_POLICY_PREF_INTERCEPTOR_UTILS_H_

namespace brave_policy {

// Populates PolicyPrefInterceptorList with the set of pref names that do not
// support dynamic policy refresh. Must be called once, early in process
// startup, before any PolicyPrefInterceptor is used.
void InitializePolicyPrefInterceptorList();

}  // namespace brave_policy

#endif  // BRAVE_BROWSER_POLICY_POLICY_PREF_INTERCEPTOR_UTILS_H_
