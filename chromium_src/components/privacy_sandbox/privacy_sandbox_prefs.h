/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PRIVACY_SANDBOX_PRIVACY_SANDBOX_PREFS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PRIVACY_SANDBOX_PRIVACY_SANDBOX_PREFS_H_

#include "src/components/privacy_sandbox/privacy_sandbox_prefs.h"

namespace prefs {

// The following prefs have been deprecated and privated, however in Brave it
// is necessary to keep these prefs visible while they are deprecated to make
// sure these modes are not enabled.
inline constexpr char kPrivacySandboxApisEnabledV2[] =
    "privacy_sandbox.apis_enabled_v2";
inline constexpr char kPrivacySandboxManuallyControlledV2[] =
    "privacy_sandbox.manually_controlled_v2";

}  // namespace privacy_sandbox

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PRIVACY_SANDBOX_PRIVACY_SANDBOX_PREFS_H_
