/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/brave_profile_policy_provider.h"

// Stash the profile path that StartupData::CreateServicesInternal is about to
// use for the profile policy connector. The BRAVE_PROFILE_POLICY_CONNECTOR_INIT
// macro (in chromium_src/chrome/browser/policy/profile_policy_connector.cc)
// consumes the stash to call SetProfileID on the BraveProfilePolicyProvider
// before its Init() runs, so the provider's bundle is populated before
// PolicyServiceImpl's constructor performs its synchronous merge.
//
// This is the Android-specific entry point. On desktop, the equivalent is set
// in chromium_src/chrome/browser/policy/profile_policy_connector_builder.cc.
#define BRAVE_STARTUP_DATA_PRE_PROFILE_POLICY_CONNECTOR \
  brave_policy::BraveProfilePolicyProvider::SetPendingProfilePath(path);
// Defensive clear: the macro normally consumes the stash, but if for any
// reason it didn't fire, don't leak the path into a subsequent profile.
#define BRAVE_STARTUP_DATA_POST_PROFILE_POLICY_CONNECTOR \
  brave_policy::BraveProfilePolicyProvider::TakePendingProfilePath();

#include <chrome/browser/startup_data.cc>

#undef BRAVE_STARTUP_DATA_PRE_PROFILE_POLICY_CONNECTOR
#undef BRAVE_STARTUP_DATA_POST_PROFILE_POLICY_CONNECTOR
