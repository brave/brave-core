/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PRIVACY_SANDBOX_TRACKING_PROTECTION_ONBOARDING_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PRIVACY_SANDBOX_TRACKING_PROTECTION_ONBOARDING_H_

#define MaybeMarkModeBEligible           \
  MaybeMarkModeBEligible_ChromiumImpl(); \
  void MaybeMarkModeBEligible

#include "src/components/privacy_sandbox/tracking_protection_onboarding.h"  // IWYU pragma: export
#undef MaybeMarkModeBEligible

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PRIVACY_SANDBOX_TRACKING_PROTECTION_ONBOARDING_H_
