/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/password_manager/android/password_manager_util_bridge.h"

#define IsInternalBackendPresent() \
  IsInternalBackendPresent() ||    \
      LoginDbDeprecationRunnerFactory::GetForProfile(profile) == nullptr
#include "src/chrome/browser/password_manager/profile_password_store_factory.cc"
#undef IsInternalBackendPresent
